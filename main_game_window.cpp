#include <QDebug>
#include <QSound>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QLine>
#include "main_game_window.h"
#include "ui_main_game_window.h"

// --------- 全局变量 --------- //
const int kIconSize = 72;
const int kTopMargin = 40;
const int kLeftMargin = 40;

const QString kIconReleasedStyle = "";
const QString kIconClickedStyle = "border: 3px ridge #FF0000;"
                                  "background-color:rgba(255, 255, 12, 0.5);"
                                  "border-radius:10px;"
                                  "padding-left:6px;"
                                  "padding-top:6px";// 点击的样子



const int kGameTimeTotal = 5 * 60 * 1000; // 总时间
const int kGameTimerInterval = 300;
const int kLinkTimerDelay = 700;

bool isPlaying;
// -------------------------- //

// 游戏主界面
MainGameWindow::MainGameWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainGameWindow),
    game(0),
    gameTimer(0),
    totalscore(0),
    score(0)
{
    this->setObjectName("dialog");
    ui->setupUi(this);
}


void MainGameWindow::startGame() {
    ui->stage->show();
    if(!isPlaying) {
        //开始游戏
        isPlaying = true;
        preIcon = NULL;
        curIcon = NULL;

        // 重载eventfilter安装到当前window（其实如果不适用ui文件的话直接可以在window的paintevent里面画）
        ui->centralWidget->installEventFilter(this);

    //    setFixedSize(kLeftMargin * 2 + (kIconMargin + kIconSize) * MAX_COL, kTopMargin + (kIconMargin + kIconSize) * MAX_ROW);
        // 选关信号槽
        connect(ui->actionBasic, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
        connect(ui->actionMedium, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
        connect(ui->actionHard, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));

        // 初始化游戏
        initGame(BASIC);
    } else {
        game->gameStatus = PLAYING;
        gameTimer->start();
    }


}

void MainGameWindow::pauseGame() {
    ui->stage->hide();
    gameTimer->stop();
    game->gameStatus = PAUSE;
}

MainGameWindow::~MainGameWindow()
{
    if (game)
        delete game;

    delete ui;
}

void MainGameWindow::initGame(GameLevel level)
{
    // 启动游戏

    game = new GameModel;
    game->startGame(level);

    // 添加button
    for(int i = 0; i < MAX_ROW * MAX_COL; i++)
    {
        imageButton[i] = new IconButton(ui->stage);
        imageButton[i]->setGeometry(kLeftMargin + (i % MAX_COL) * kIconSize, kTopMargin + (i / MAX_COL) * kIconSize, kIconSize, kIconSize);
        // 设置索引
        imageButton[i]->xID = i % MAX_COL;
        imageButton[i]->yID = i / MAX_COL;

        imageButton[i]->show();

        if (game->getGameMap()[i])
        {
            // 有方块就设置图片
            QPixmap iconPix;
            QString fileString;
            fileString.sprintf(":/res/image/%d.png", game->getGameMap()[i]);
            iconPix.load(fileString);
            QIcon icon(iconPix);
            imageButton[i]->setIcon(icon);
            imageButton[i]->setIconSize(QSize(kIconSize, kIconSize));

            // 添加按下的信号槽
            connect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
        }
        else
            imageButton[i]->hide();
    }

    // 进度条
    ui->timeBar->setMaximum(kGameTimeTotal);
    ui->timeBar->setMinimum(0);
    ui->timeBar->setValue(kGameTimeTotal);

    // 游戏计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(gameTimerEvent()));
    gameTimer->start(kGameTimerInterval);

    // 连接状态值
    isLinking = false;

    // 播放背景音乐(QMediaPlayer只能播放绝对路径文件),确保res文件在程序执行文件目录里而不是开发目录
    audioPlayer = new QMediaPlayer(this);
    QString curDir = QCoreApplication::applicationDirPath(); // 这个api获取路径在不同系统下不一样,mac 下需要截取路径
    QStringList sections = curDir.split(QRegExp("[/]"));
    QString musicPath;

    for (int i = 0; i < sections.size() - 3; i++)
        musicPath += sections[i] + "/";

    audioPlayer->setMedia(QUrl::fromLocalFile(musicPath + "res/sound/backgrand.mp3"));
    audioPlayer->play();
}


GameLevel MainGameWindow::getNextLevel() {
    if(game->gameLevel==BASIC) {
        return MEDIUM;
    } else if(game->gameLevel == MEDIUM) {
        return HARD;
    } else {
        return HARD;
    }
}

void MainGameWindow::onFinished() {
    QMessageBox::information(this, "great", QString("\n     you win score : %1    ").arg(score));//胜局显示胜利并显示本局得分
    ui->lcdScore->display(totalscore += score);//显示总分

    if(game->gameLevel == HARD) {
        //询问是否再来一局
        QMessageBox * mBox = new QMessageBox(this);
        mBox->setWindowTitle("是否再来一局？");
        mBox->setText("你已经超神啦，再来玩一局吧！");
        mBox->setStandardButtons(QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel);
        mBox->setButtonText(QMessageBox::StandardButton::Ok, "好的");
        mBox->setButtonText(QMessageBox::StandardButton::Cancel, "好的");
        mBox->exec();

        QMessageBox::StandardButton ret = mBox->standardButton(mBox->clickedButton());
        createGameWithLevel(HARD);
        if (mBox != nullptr)
        {
            delete mBox;
            mBox = nullptr;
        }
        return;
    }

    //询问是否增加难度
    QMessageBox * mBox = new QMessageBox(this);
    mBox->setWindowTitle("是否增加难度？");
    //mBox->setText(QString::fromLocal8Bit("是否增加难度？"));
    //qt使用utf-8编码,现在windows好像也支持？
    mBox->setText("哇，游戏对你来说简直是小菜一碟，不如挑战更高难度？");
    mBox->setStandardButtons(QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel);
    mBox->setButtonText(QMessageBox::StandardButton::Ok, "接受挑战");
    mBox->setButtonText(QMessageBox::StandardButton::Cancel, "算了");
    mBox->exec();

    QMessageBox::StandardButton ret = mBox->standardButton(mBox->clickedButton());
    switch (ret) {
        case QMessageBox::NoButton:
            //something to do
            break;
        case QMessageBox::Ok:
            createGameWithLevel(getNextLevel());
            break;
        case QMessageBox::Cancel:
            createGameWithLevel(game->gameLevel);
            break;
        default:
            break;
    }
    if (mBox != nullptr)
    {
        delete mBox;
        mBox = nullptr;
    }
}

void MainGameWindow::onIconButtonPressed()
{
    // 如果当前有方块在连接，不能点击方块
    // 因为涉及到多线，可能还要维护队列，有点复杂，就先这么简单处理一下
    if (isLinking)
    {
        // 播放音效
        QSound::play(":/res/sound/release.wav");
        return;
    }


    // 记录当前点击的icon
    curIcon = dynamic_cast<IconButton *>(sender());

    if(!preIcon)
    {
        // 播放音效
        QSound::play(":/res/sound/select.wav");

        // 如果单击一个icon
        curIcon->setStyleSheet(kIconClickedStyle);
        preIcon = curIcon;
    }
    else
    {
        if(curIcon != preIcon)
        {
            // 如果不是同一个button就都标记,尝试连接
            curIcon->setStyleSheet(kIconClickedStyle);
            if(game->linkTwoTiles(preIcon->xID, preIcon->yID, curIcon->xID, curIcon->yID))
            {
                // 锁住当前状态
                isLinking = true;

                // 播放音效
                QSound::play(":/res/sound/pair.wav");

                // 重绘
                update();

                // 延迟后实现连接效果
                QTimer::singleShot(kLinkTimerDelay, this, SLOT(handleLinkEffect()));

                // 每次检查一下是否僵局或胜利
                if (game->isWin()){
                    onFinished();
                }else if (game->isFrozen()){
                    QMessageBox::information(this, "oops", "dead game");
                }

            }
            else
            {
                // 播放音效
                QSound::play(":/res/sound/release.wav");

                // 消除失败，恢复
                preIcon->setStyleSheet(kIconReleasedStyle);
                curIcon->setStyleSheet(kIconReleasedStyle);

                // 指针置空，用于下次点击判断
                preIcon = NULL;
                curIcon = NULL;
            }
        }
        else if(curIcon == preIcon)
        {
            // 播放音效
            QSound::play(":/res/sound/release.wav");

            preIcon->setStyleSheet(kIconReleasedStyle);
            curIcon->setStyleSheet(kIconReleasedStyle);
            preIcon = NULL;
            curIcon = NULL;
        }
    }
}

void MainGameWindow::handleLinkEffect()
{
    // 消除成功，隐藏掉，并析构
    game->paintPoints.clear();
    preIcon->hide();
    curIcon->hide();

    preIcon = NULL;
    curIcon = NULL;

    // 重绘
    update();

    // 恢复状态
    isLinking = false;
}

bool MainGameWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 重绘时会调用，可以手动调用
    if (event->type() == QEvent::Paint)
    {
        QPainter painter(ui->stage);
        QPen pen;
        QColor color(rand() % 256, rand() % 256, rand() % 256);
        pen.setColor(color);
        pen.setWidth(5);
        painter.setPen(pen);

        QString str;
        for (int i = 0; i < game->paintPoints.size(); i++)
        {
            PaintPoint p = game->paintPoints[i];
            str += "x:" + QString::number(p.x) + "y:" + QString::number(p.y) + "->";
        }
//        qDebug() << str;

        // 连接各点画线（注，qt中用标砖vector的size好像有点问题，需要类型转换，否则溢出）
        for (int i = 0; i < int(game->paintPoints.size()) - 1; i++)
        {
            PaintPoint p1 = game->paintPoints[i];
            PaintPoint p2 = game->paintPoints[i + 1];

            // 拿到各button的坐标,注意边缘点坐标
            QPoint btn_pos1;
            QPoint btn_pos2;

            // p1
            if (p1.x == -1)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + 0]->pos();
                btn_pos1 = QPoint(btn_pos1.x() - kIconSize, btn_pos1.y());
            }
            else if (p1.x == MAX_COL)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos1 = QPoint(btn_pos1.x() + kIconSize, btn_pos1.y());
            }
            else if (p1.y == -1)
            {
                btn_pos1 = imageButton[0 + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() - kIconSize);
            }
            else if (p1.y == MAX_ROW)
            {
                btn_pos1 = imageButton[(MAX_ROW - 1) * MAX_COL + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() + kIconSize);
            }
            else
                btn_pos1 = imageButton[p1.y * MAX_COL + p1.x]->pos();

            // p2
            if (p2.x == -1)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + 0]->pos();
                btn_pos2 = QPoint(btn_pos2.x() - kIconSize, btn_pos2.y());
            }
            else if (p2.x == MAX_COL)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos2 = QPoint(btn_pos2.x() + kIconSize, btn_pos2.y());
            }
            else if (p2.y == -1)
            {
                btn_pos2 = imageButton[0 + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() - kIconSize);
            }
            else if (p2.y == MAX_ROW)
            {
                btn_pos2 = imageButton[(MAX_ROW - 1) * MAX_COL + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() + kIconSize);
            }
            else
                btn_pos2 = imageButton[p2.y * MAX_COL + p2.x]->pos();



            // 中心点
            QPoint pos1(btn_pos1.x() + kIconSize / 2, btn_pos1.y() + kIconSize / 2);
            QPoint pos2(btn_pos2.x() + kIconSize / 2, btn_pos2.y() + kIconSize / 2);

            painter.drawLine(pos1, pos2);
        }

        return true;
    }
    else
        return QMainWindow::eventFilter(watched, event);
}

void MainGameWindow::gameTimerEvent()
{
    // 进度条计时效果
    if(ui->timeBar->value() == 0)
    {
        gameTimer->stop();
        QMessageBox::information(this, "game over", "play again>_<");
    }
    else
    {
        ui->timeBar->setValue(ui->timeBar->value() - kGameTimerInterval);
        score = ui->timeBar->value();//更新得分
    }

}

/*
void MainGameWindow::on_robot_btn_clicked()
{

    while (game->gameStatus == PLAYING)
    {
        // 连接生成提示

        int srcX = game->getHint()[0];
        int srcY = game->getHint()[1];
        int dstX = game->getHint()[2];
        int dstY = game->getHint()[3];

        if(game->linkTwoTiles(srcX, srcY, dstX, dstY))
        {
            // 播放音效
//            QSound::play(":/res/sound/pair.wav");

            // 消除成功，隐藏掉
            IconButton *icon1 = imageButton[srcY * MAX_COL + srcX];
            IconButton *icon2 = imageButton[dstY * MAX_COL + dstX];

            icon1->hide();
            icon2->hide();

            game->paintPoints.clear();

            // 重绘
            update();

            // 检查是否胜利
            if (game->isWin()) {
                onFinished();
            }
            // 每次检查一下是否僵局
            if (!game->isWin() && game->isFrozen() && game->gameStatus == PLAYING)
                QMessageBox::information(this, "Oops", "dead game");

        }
    }
}
*/

void MainGameWindow::createGameWithLevel(GameLevel level)
{
    // 先析构之前的
    if (game)
    {
        delete game;
        for (int i = 0;i < MAX_ROW * MAX_COL; i++)
        {
            if (imageButton[i])
               delete imageButton[i];
        }
    }

    // 停止音乐
    audioPlayer->stop();

    initGame(level);

}

void MainGameWindow::on_btn_start_clicked()
{
    startGame();
}

void MainGameWindow::on_btn_pause_clicked()
{
    pauseGame();
}
