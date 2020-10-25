#include "servergamestate.h"

// Default constructor
ServerGameState::ServerGameState() {}

// Initialises all attributes to values for start of match
ServerGameState::ServerGameState(PlayerPosition dealer)
{
    // Initialise attributes
    phase = BIDDING;
    gameNumber = 1;
    dealNumber = 0;
    trickNumber = 0;
    teamVulnerable[N_S] = false;
    teamVulnerable[E_W] = false;
    passCount = 0;
    this->dealer = dealer;
    playerTurn = dealer;
    handToPlay = NORTH;
    declarer = NORTH;

    // Initialise player hands
    for(qint8 player = NORTH; player <= WEST; ++player)
        playerHands.insert(PlayerPosition(player), CardSet());

    // Populate card set with all of the 52 different cards
    for(qint8 suitVal = CLUBS; suitVal <= SPADES; suitVal++){
        CardSuit suit = CardSuit(suitVal);
        for(qint8 rankVal = TWO; rankVal <= ACE; rankVal++){
            CardRank rank = CardRank(rankVal);
            Card card(suit, rank);
            deck.addCard(card);
        }
    }
}

void ServerGameState::startGame()
{
    nextDeal();
}

// Prepare game for next deal round
// Reset game specific attributes, shuffle deck and deal cards
void ServerGameState::nextDeal()
{
    // Select player to left of current dealer as new dealer
    if(dealNumber > 0 || gameNumber > 1)
        dealer = PlayerPosition((dealer + 1) % 4);

    // Reset attributes
    phase = BIDDING;
    currentBid = nullptr;
    contractBid = nullptr;
    ++ dealNumber;
    trickNumber = 0;
    tricks.clear();
    passCount = 0;

    // Clear player hands
    for(qint8 player = NORTH; player <= WEST; ++ player)
        playerHands[PlayerPosition(player)].clearSet();

    // Shuffle deck
    deck.shuffle();

    // Deal cards in clockwise direction
    // Select player to dealer's left as first player to receive a card
    PlayerPosition targetPlayer = PlayerPosition((dealer + 1) % 4);
    while(deck.getCardCount() > 0){
        playerHands[targetPlayer].addCard(deck.removeTopCard());
    }

    // Select dealer for first turn
    playerTurn = dealer;
}

// Prepare game for next trick
void ServerGameState::nextTrick()
{
    ++ trickNumber;
    tricks.append(CardSet());
}


// Update the game state based on the latest bid made
// Does nothing if the bid is invalid. Bid validity must be checked seperately prior to
// calling this function isBidValid()
void ServerGameState::updateBidState(const Bid &bid)
{
    // Count number of passes in a row
    if(bid.getCall() == PASS){
        ++ passCount;
    }
    // Check if bid is valid
    else if(isBidValid(bid)){
        // Reset pass counter
        passCount = 0;

        // Update bid
        if(bid.getCall() == DOUBLE_BID || bid.getCall() == REDOUBLE_BID){
            currentBid->setCall(bid.getCall());
        }
        else{
            delete currentBid;
            currentBid = new Bid(bid);
        }
    }
    else {
        return;
    }

    // Select player to left of most recent player to play for next turn
    playerTurn = PlayerPosition((playerTurn + 1) % 4);

    // Check if bid has been made
    if(currentBid == nullptr){
        // Redeal cards if 4 passes have been made given no bid has been made
        if(passCount == 4)
            nextDeal();
    }
    else{
        // Check if 3 passes have been made given a bid has been made
        if(passCount == 3){
            // Transition to play phase
            phase = CARDPLAY;
            contractBid = currentBid;
            currentBid = nullptr;
            declarer = contractBid->getBidder();
            // Select player to left of declarer for first turn
            playerTurn = PlayerPosition((declarer + 1) % 4);
            handToPlay = playerTurn;
            nextTrick();
        }
    }
}

// Update the game state based on the latest card played by the player
// Assumes card is played from handToPlay by the PlayerTurn player
// Does nothing if the bid is invalid. Card validity must be checked seperately prior to
// calling this function using isCardValid()
void ServerGameState::updatePlayState(const Card &card)
{
    // Check if card played is valid
    if(!isCardValid(card))
        return;

    // Play card
    CardSet* currentTrick = &tricks[trickNumber - 1];
    currentTrick->addCard(card);

    // Remove card from player hand
    qint8 removeIndex = 0;
    while(!(playerHands[handToPlay].getCard(removeIndex) == card))
        removeIndex++;
    playerHands[handToPlay].removeCard(removeIndex);

    // Check if trick is complete
    if(currentTrick->getCardCount() == 4){
        // Determine winner
        PlayerPosition winner = determineTrickWinner();

        // TO ADD: UPDATE SCORE

        // Check if deal round is complete
        if(tricks.size() == 13){
            nextDeal();
            return;
        }

        // Winner plays first next
        handToPlay = winner;
        if(handToPlay == getDummy())
            playerTurn = declarer;
        else
            playerTurn = handToPlay;
        nextTrick();
    }
    // Get next hand to play and player positoin
    else{
        handToPlay = PlayerPosition((handToPlay + 1) % 4);
        if(handToPlay == getDummy())
            playerTurn = declarer;
        else
            playerTurn = handToPlay;
    }
}

// Getter for the cards currently in the deck
const CardSet& ServerGameState::getDeck()
{
    return deck;
}

// Generate and return the game state tailored for the player
PlayerGameState ServerGameState::getPlayerGameState(PlayerPosition player, QVector<Player*> players, GameEvent gameEvent)
{
    // Create player positions map and card count map
    QMap<PlayerPosition, QString> playerPositions;
    QMap<PlayerPosition, qint8> playerCardCount;
    for(const Player* player: players){
        playerPositions.insert(player->getPosition(), player->getPlayerName());
        playerCardCount.insert(player->getPosition(), playerHands.value(player->getPosition()).getCardCount());
    }

    // Initialise dummy hand sent to player
    CardSet dummyHand;
    if(phase == CARDPLAY)
        dummyHand = playerHands[getDummy()];
    return PlayerGameState(*this, gameEvent, playerPositions, playerCardCount, playerHands[player], dummyHand);
}

// Getter for player hands
const QMap<PlayerPosition, CardSet>& ServerGameState::getPlayerHands() const
{
    return playerHands;
}

// Setter for player hands
void ServerGameState::setPlayerHands(const QMap<PlayerPosition, CardSet> &playerHands)
{
    this->playerHands = playerHands;
}

// Check if the new bid is valid given the current bid. Passing nullptr as the current bid
// argument implies there is no current bid
bool ServerGameState::isBidValid(const Bid &bid) const
{
    if(bid.getCall() == PASS){
        return true;
    }
    // Check if new bid is valid given no bid has been made yet
    else if(currentBid == nullptr){
        return bid.getCall() == BID;
    }
    else{
        Team currentBidderTeam = getPlayerTeam(currentBid->getBidder());
        Team newBidderTeam = getPlayerTeam(bid.getBidder());
        if(bid.getCall() == DOUBLE_BID){
            // Double is invalid if bid was made by a member of the same team
            return newBidderTeam != currentBidderTeam && currentBid->getCall() == BID;
        }
        else if(bid.getCall() == REDOUBLE_BID){
            // Redouble is invalid if the bid was made by a member of the opposing team or
            // the bid was not doubled
            return newBidderTeam == currentBidderTeam && currentBid->getCall() == DOUBLE_BID;
        }
        // Check if new bid is a higher bid than the current bid
        else if(bid > *currentBid){
            return true;
        }
    }
    return false;
}

// Check if the card is valid to play in the current trick. Assumes the card is played from
// the hand indicated by handToPlay
bool ServerGameState::isCardValid(const Card &card) const
{
    // Check if card is in hand to play
    if(!playerHands[handToPlay].containsCard(card))
        return false;

    // Check if any cards have been played in the current trick
    CardSet trick = tricks[trickNumber - 1];
    if(trick.getCardCount() == 0)
        return true;

    // Check if the card matches the suit of the first card played in the trick
    if(card.getSuit() == trick.getCard(0).getSuit())
        return true;
    // Check if player had first suit played in their hand
    else if(!playerHands[handToPlay].containsSuit(trick.getCard(0).getSuit()))
        return true;
    return false;
}

// Get the team the player belongs to based on their position
Team ServerGameState::getPlayerTeam(PlayerPosition player){
    switch (player) {
        case NORTH:
        case SOUTH:
            return N_S;
        default:
            return E_W;
    }
}

// Determine the player which won the most recently played trick
PlayerPosition ServerGameState::determineTrickWinner() const{
    const CardSet &trick = tricks[trickNumber - 1];
    qint8 bestIndex = 0;
    for(qint8 index = 1; index < 4; ++ index){
        const Card &bestCard = trick.getCard(bestIndex);
        const Card &card = trick.getCard(index);
        CardSuit trumpSuit = contractBid->getTrumpSuit();
        bool updateBestCard = false;

        // Check if card suit is trump suit
        if(card.getSuit() == trumpSuit && bestCard.getSuit() != trumpSuit)
            updateBestCard = true;
        // Check if card is better than best card if the same suit
        else if(card.getSuit() == bestCard.getSuit() && bestCard < card)
            updateBestCard = true;
        // Update best card
        if(updateBestCard)
            bestIndex = index;
    }

    // Identify winning player
    // playerTurn refers to player that played last card in trick
    // Add 1 to get player that played first card then add bestIndex to get winning player
    return PlayerPosition((handToPlay + bestIndex + 1) % 4);
}
