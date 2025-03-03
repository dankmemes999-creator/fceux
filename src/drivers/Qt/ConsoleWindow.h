
//

#ifndef __GameAppH__
#define __GameAppH__

#include <vector>
#include <string>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QCursor>

#include "Qt/ConsoleViewerGL.h"
#include "Qt/ConsoleViewerSDL.h"
#include "Qt/GamePadConf.h"

class  emulatorThread_t : public QThread
{
	Q_OBJECT

	protected:
		void run( void ) override;

	public:
		emulatorThread_t(void);

		void setPriority( QThread::Priority priority );

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		int setSchedParam( int policy, int priority );
		int getSchedParam( int &policy, int &priority );
		int setNicePriority( int value );
		int getNicePriority( void );
		int getMinSchedPriority(void);
		int getMaxSchedPriority(void);
		#endif
	private:
		void init(void);

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		pthread_t  pself;
		int        pid;
		#endif

	signals:
		void finished();
};

class  consoleMenuBar : public QMenuBar
{
	public:
		consoleMenuBar(QWidget *parent = 0);
		~consoleMenuBar(void);

	protected:
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
};

class  consoleRecentRomAction : public QAction
{
	Q_OBJECT

	public:
		consoleRecentRomAction( QString title, QWidget *parent = 0);
		~consoleRecentRomAction(void);

		std::string  path;

	public slots:
		void activateCB(void);

};

class  consoleWin_t : public QMainWindow
{
	Q_OBJECT

	public:
		consoleWin_t(QWidget *parent = 0);
		~consoleWin_t(void);

		ConsoleViewGL_t   *viewport_GL;
		ConsoleViewSDL_t  *viewport_SDL;

		void setCyclePeriodms( int ms );

		QMutex *mutex;

		void requestClose(void);

	 	void QueueErrorMsgWindow( const char *msg );

		int  showListSelectDialog( const char *title, std::vector <std::string> &l );

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		int setSchedParam( int policy, int priority );
		int getSchedParam( int &policy, int &priority );
		int setNicePriority( int value );
		int getNicePriority( void );
		int getMinSchedPriority(void);
		int getMaxSchedPriority(void);
		#endif

		emulatorThread_t *emulatorThread;

		void addRecentRom( const char *rom );

		QSize  calcRequiredSize(void);

		void setViewportAspect(void);

		void loadCursor(void);
		void setViewerCursor( QCursor s );
		void setViewerCursor( Qt::CursorShape s );
		Qt::CursorShape getViewerCursor(void);
	protected:
		consoleMenuBar *menubar;

		QMenu *fileMenu;
		QMenu *optMenu;
		QMenu *emuMenu;
		QMenu *toolsMenu;
		QMenu *debugMenu;
		QMenu *movieMenu;
		QMenu *helpMenu;
		QMenu *recentRomMenu;
		
		QAction *openROM;
		QAction *closeROM;
		QAction *playNSF;
		QAction *loadStateAct;
		QAction *saveStateAct;
		QAction *quickLoadAct;
		QAction *quickSaveAct;
		QAction *loadLuaAct;
		QAction *scrShotAct;
		QAction *quitAct;
		QAction *inputConfig;
		QAction *gamePadConfig;
		QAction *gameSoundConfig;
		QAction *gameVideoConfig;
		QAction *hotkeyConfig;
		QAction *paletteConfig;
		QAction *guiConfig;
		QAction *timingConfig;
		QAction *movieConfig;
		QAction *autoResume;
		QAction *fullscreen;
		QAction *aboutAct;
		QAction *aboutActQt;
		QAction *msgLogAct;
		QAction *state[10];
		QAction *powerAct;
		QAction *resetAct;
		QAction *sresetAct;
		QAction *pauseAct;
		QAction *gameGenieAct;
		QAction *loadGgROMAct;
		QAction *insCoinAct;
		QAction *fdsSwitchAct;
		QAction *fdsEjectAct;
		QAction *fdsLoadBiosAct;
		QAction *cheatsAct;
		QAction *ramWatchAct;
		QAction *ramSearchAct;
		QAction *debuggerAct;
		QAction *codeDataLogAct;
		QAction *traceLogAct;
		QAction *hexEditAct;
		QAction *ppuViewAct;
		QAction *ntViewAct;
		QAction *ggEncodeAct;
		QAction *iNesEditAct;
		QAction *openMovAct;
		QAction *stopMovAct;
		QAction *recMovAct;
		QAction *recAsMovAct;
		QAction *region[3];
		QAction *ramInit[4];

		QTimer  *gameTimer;

		std::string errorMsg;
		bool        errorMsgValid;
		bool        closeRequested;
		bool        recentRomMenuReset;
		bool        firstResize;

		std::list <std::string*> romList;

		unsigned int updateCounter;
	protected:
		void resizeEvent(QResizeEvent *event);
		void closeEvent(QCloseEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		void syncActionConfig( QAction *act, const char *property );
		void showErrorMsgWindow(void);

	private:
		void createMainMenu(void);
		void buildRecentRomMenu(void);
		void saveRecentRomMenu(void);
		void clearRomList(void);
		void setRegion(int region);

	public slots:
		void openDebugWindow(void);
		void openHexEditor(void);
		void openGamePadConfWin(void);
		void toggleFullscreen(void);
		void toggleMenuVis(void);
	private slots:
		void closeApp(void);
		void openROMFile(void);
		void loadNSF(void);
		void loadStateFrom(void);
		void saveStateAs(void);
		void quickLoad(void);
		void quickSave(void);
		void closeROMCB(void);
		void aboutFCEUX(void);
		void aboutQt(void);
		void openOnlineDocs(void);
		void openMsgLogWin(void);
		void openInputConfWin(void);
		void openGameSndConfWin(void);
		void openGameVideoConfWin(void);
		void openHotkeyConfWin(void);
		void openPaletteConfWin(void);
		void openGuiConfWin(void);
		void openTimingConfWin(void);
		void openPaletteEditorWin(void);
		void openTimingStatWin(void);
		void openMovieOptWin(void);
		void openCodeDataLogger(void);
		void openTraceLogger(void);
		void toggleAutoResume(void);
		void updatePeriodic(void);
		void changeState0(void);
		void changeState1(void);
		void changeState2(void);
		void changeState3(void);
		void changeState4(void);
		void changeState5(void);
		void changeState6(void);
		void changeState7(void);
		void changeState8(void);
		void changeState9(void);
		void loadLua(void);
		void takeScreenShot(void);
		void powerConsoleCB(void);
		void consoleHardReset(void);
		void consoleSoftReset(void);
		void consolePause(void);
		void toggleGameGenie(bool checked);
		void loadGameGenieROM(void);
		void setRegionNTSC(void);
		void setRegionPAL(void);
		void setRegionDendy(void);
		void setRamInit0(void);
		void setRamInit1(void);
		void setRamInit2(void);
		void setRamInit3(void);
		void insertCoin(void);
		void fdsSwitchDisk(void);
		void fdsEjectDisk(void);
		void fdsLoadBiosFile(void);
		void emuSpeedUp(void);
		void emuSlowDown(void);
		void emuSlowestSpd(void);
		void emuNormalSpd(void);
		void emuFastestSpd(void);
		void emuCustomSpd(void);
		void emuSetFrameAdvDelay(void);
		void openPPUViewer(void);
		void openNTViewer(void);
		void openGGEncoder(void);
		void openNesHeaderEditor(void);
		void openCheats(void);
		void openRamWatch(void);
		void openRamSearch(void);
		void openMovie(void);
		void stopMovie(void);
		void recordMovie(void);
		void recordMovieAs(void);
		void setAutoFireOnFrames(void);
		void setAutoFireOffFrames(void);

};

extern consoleWin_t *consoleWindow;

#endif
