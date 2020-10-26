#include "entrywindow.h"
#include "ui_entrywindow.h"

EntryWindow::EntryWindow(QWidget *parent) :QWidget(parent), ui(new Ui::EntryWindow)
{
    ui->setupUi(this);
    setupWindow();
    staticGUIElements();
    //Ensure the window is open by showing it after the window is setup.
    this->show();
}

EntryWindow::~EntryWindow()
{
    qDebug() << "Exit program.";
    delete ui;
}
// The setupWindow is used to setup the GUI background of the windows as well as
// the size of the window, which is fixed.
void EntryWindow::setupWindow()
{
    // QPixmap maps the pixels of the background fot the palatte to brush it onto the
    // current ui.
    QPixmap bkgnd(":/resources/guiResources/background/background2.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Background, bkgnd);
    this->setPalette(palette);

    this->setFixedSize(640,480); //Fix the size of the window to a specified 640 by 480 ratio (16:12)
    this->setWindowTitle ("Rubber Contract Bridge");
}
// The buttons, labels and lists are created here for the specific window.
void EntryWindow::staticGUIElements()
{
    // The buttons, labels and lists are created here for the specific window.
    // Each of the varius elements need their own Pixelmap.
    QPixmap clientPixel(":/resources/guiResources/buttons/c_grey.png");
    QPixmap serverPixel(":/resources/guiResources/buttons/s_grey.png");

    //Server
    Hover *serverButton = new Hover(this->pageID,1,this);
    serverButton->setPixmap(serverPixel);
    serverButton->setGeometry(70,300,150,64);
    serverButton->setObjectName("Server Button");
    connect(serverButton,&Hover::gotoServerLogin,this,&EntryWindow::goServerLogin);
    //Client
    Hover *clientButton = new Hover(this->pageID,2,this);
    clientButton->setPixmap(clientPixel);
    clientButton->setGeometry(420,300,150,64);
    clientButton->setObjectName("Client Button");
    connect(clientButton,&Hover::gotoClientLogin,this,&EntryWindow::goClientLogin);
    //Background
    QLabel *title = new QLabel(this);
    QFont westernFont("Montague", 36);
    title->setFont(westernFont);
    title->setText("RUBBER CONTRACT BRIDGE");
    title->setGeometry(50,80,600,100);
    title->setStyleSheet("color: white");
}

void EntryWindow::goServerLogin()
{
    // This application will act as server.
    serverLogin = new ServerLogin();
    this->close();
}

void EntryWindow::goClientLogin()
{
    // This application will act as client.
     clientLogin = new ClientLogin();
     this->close();
}

int EntryWindow::getPageID()
{
    return pageID;
}
