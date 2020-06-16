#ifndef MAIN_GAME_WINDOW_H
#define MAIN_GAME_WINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QWidget>
#include <QTimer>
#include <QMediaPlayer>
#include "game_model.h"

namespace Ui {
class MainGameWindow;
}

// 继承自button，存储坐标值
struct IconButton : QPushButton
{
public:
    IconButton(QWidget *parent = Q_NULLPTR) :
        QPushButton(parent),
        xID(-1),
        yID(-1)
    {
    }
    int xID; // x 坐标
    int yID; // y 坐标
};

class MainGameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainGameWindow(QWidget *parent = 0);
    virtual ~MainGameWindow();

    virtual bool eventFilter(QObject *watched, QEvent *event); // 事件过滤

private:
    Ui::MainGameWindow *ui;
    GameModel *game; // 游戏模型
    IconButton *imageButton[MAX_ROW * MAX_COL]; // 图片button数组
    QTimer *gameTimer; // 游戏计时器
    IconButton *preIcon, *curIcon; // 记录点击的icon
    bool isLinking; // 维持一个连接状态的标志

    QMediaPlayer *audioPlayer; // 音乐播放器

    qint32 totalscore;  //截至本局的totalscore
    qint32 score; //本局得分
    void initGame(GameLevel level); // 初始化游戏
    void startGame();
    void pauseGame();
    GameLevel getNextLevel();
    void onFinished();

private slots:
    void onIconButtonPressed(); // icon点击到响应
    void gameTimerEvent(); // 游戏计时回调
    void handleLinkEffect(); // 实现连接效果
    //void on_robot_btn_clicked(); // 机器人自动刷
    void createGameWithLevel(GameLevel level); // 选中难度开始

    void on_btn_start_clicked();
    void on_btn_pause_clicked();
};

#endif // MAIN_GAME_WINDOW_H
