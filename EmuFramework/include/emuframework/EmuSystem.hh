#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/IO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/audio/Audio.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/audio/PcmFormat.hh>
#include <imagine/util/string.h>
#include <system_error>
#include <experimental/optional>
#include <emuframework/EmuVideo.hh>

#ifdef ENV_NOTE
#define PLATFORM_INFO_STR ENV_NOTE " (" CONFIG_ARCH_STR ")"
#else
#define PLATFORM_INFO_STR "(" CONFIG_ARCH_STR ")"
#endif
#define CREDITS_INFO_STRING "Built : " __DATE__ "\n" PLATFORM_INFO_STR "\n\n"

#if (defined __ANDROID__ && !defined CONFIG_MACHINE_OUYA) || \
	defined CONFIG_BASE_IOS || \
	(defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA)
#define CONFIG_VCONTROLS_GAMEPAD
#endif

class EmuInputView;

struct AspectRatioInfo
{
	constexpr AspectRatioInfo(const char *name, uint n, uint d): name(name), aspect{n, d} {}
	const char *name;
	IG::Point2D<uint> aspect;
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", 1, 1}, {"Full Screen", 0, 1}

struct BundledGameInfo
{
	const char *displayName;
	const char *assetName;
};

using EmuNavView = BasicNavView;

enum { STATE_RESULT_OK, STATE_RESULT_NO_FILE, STATE_RESULT_NO_FILE_ACCESS, STATE_RESULT_IO_ERROR,
	STATE_RESULT_INVALID_DATA, STATE_RESULT_OTHER_ERROR };

class EmuSystem
{
private:
	static FS::PathString gamePath_, fullGamePath_;
	static FS::FileString gameName_, fullGameName_;
	static FS::PathString defaultSavePath_;
	static FS::PathString gameSavePath_;

public:
	enum class State
	{
		OFF,
		STARTING,
		PAUSED,
		ACTIVE
	};

	enum class ViewID
	{
		MAIN_MENU,
		VIDEO_OPTIONS,
		AUDIO_OPTIONS,
		INPUT_OPTIONS,
		SYSTEM_OPTIONS,
		GUI_OPTIONS,
		EDIT_CHEATS,
		LIST_CHEATS,
	};

	enum class LoadProgress : uint8
	{
		UNSET,
		FAILED,
		OK,
		SET,
		UPDATE
	};

	struct LoadProgressMessage
	{
		constexpr LoadProgressMessage() {}
		constexpr LoadProgressMessage(LoadProgress progress, int intArg, int intArg2, int intArg3):
			intArg{intArg}, intArg2{intArg2}, intArg3{intArg3}, progress{progress} {}
		int intArg{};
		int intArg2{};
		int intArg3{};
		LoadProgress progress{LoadProgress::UNSET};
	};

	using OnLoadProgressDelegate = DelegateFunc<bool(int pos, int max, const char *label)>;

	using Error = std::experimental::optional<std::runtime_error>;
	using NameFilterFunc = bool(*)(const char *name);
	static State state;
	static FS::PathString savePath_;
	static Base::Timer autoSaveStateTimer;
	static int saveStateSlot;
	static Base::FrameTimeBase startFrameTime;
	static Base::FrameTimeBase timePerVideoFrame;
	static uint emuFrameNow;
	static bool runFrameOnDraw;
	static Audio::PcmFormat pcmFormat;
	static uint audioFramesPerVideoFrame;
	static uint aspectRatioX, aspectRatioY;
	static const uint maxPlayers;
	static const AspectRatioInfo aspectRatioInfo[];
	static const uint aspectRatioInfos;
	static const char *configFilename;
	static const char *inputFaceBtnName;
	static const char *inputCenterBtnName;
	static const uint inputCenterBtns;
	static const uint inputFaceBtns;
	static const bool inputHasTriggerBtns;
	static const bool inputHasRevBtnLayout;
	static bool inputHasKeyboard;
	static bool inputHasOptionsView;
	static bool hasBundledGames;
	static bool hasPALVideoSystem;
	enum VideoSystem { VIDSYS_NATIVE_NTSC, VIDSYS_PAL };
	static double frameTimeNative;
	static double frameTimePAL;
	static bool hasResetModes;
	enum ResetMode { RESET_HARD, RESET_SOFT };
	static bool handlesArchiveFiles;
	static bool handlesGenericIO;
	static bool hasCheats;
	static NameFilterFunc defaultFsFilter;
	static NameFilterFunc defaultBenchmarkFsFilter;
	static const char *creditsViewStr;

	static CallResult onInit();
	static void onMainWindowCreated(Base::Window &win);
	static void onCustomizeNavView(EmuNavView &view);
	static View *makeView(ViewAttachParams attach, ViewID id);
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static void cancelAutoSaveStateTimer();
	static void startAutoSaveStateTimer();
	static std::system_error loadState(int slot = saveStateSlot);
	static std::error_code saveState();
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(uint idx);
	static const char *gamePath() { return gamePath_.data(); }
	static const char *fullGamePath() { return fullGamePath_.data(); }
	static FS::FileString gameName() { return gameName_; }
	static FS::FileString fullGameName() { return strlen(fullGameName_.data()) ? fullGameName_ : gameName_; }
	static FS::FileString gameFileName() { return FS::basename(fullGamePath_); }
	static void setFullGameName(const char *name) { string_copy(fullGameName_, name); }
	static FS::FileString fullGameNameForPathDefaultImpl(const char *path);
	static FS::FileString fullGameNameForPath(const char *path);
	static void makeDefaultSavePath();
	static const char *defaultSavePath();
	static const char *savePath();
	static FS::PathString sprintStateFilename(int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName_.data());
	static bool loadAutoState();
	static void saveAutoState();
	static void saveBackupMem();
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void initOptions();
	static void onOptionsLoaded();
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, uint key, uint readSize);
	static void createWithMedia(GenericIO io, const char *path, const char *name,
		Error &err, OnLoadProgressDelegate onLoadProgress);
	static Error loadGame(IO &io, OnLoadProgressDelegate onLoadProgress);
	static FS::PathString willLoadGameFromPath(FS::PathString path);
	static Error loadGameFromPath(const char *path, OnLoadProgressDelegate onLoadProgress);
	static Error loadGameFromFile(GenericIO io, const char *name, OnLoadProgressDelegate onLoadProgress);
	static YesNoAlertView *makeYesNoAlertView(const char *label, const char *choice1 = {}, const char *choice2 = {});
	[[gnu::hot]] static void runFrame(EmuVideo &video, bool renderGfx, bool processGfx, bool renderAudio);
	static void skipFrames(uint frames);
	static void onPrepareVideo(EmuVideo &video);
	static bool vidSysIsPAL();
	static double frameTime();
	static double frameTime(VideoSystem system);
	static double defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, double time);
	static bool setFrameTime(VideoSystem system, double time);
	static uint multiresVideoBaseX();
	static uint multiresVideoBaseY();
	static void configAudioRate(double frameTime);
	static void configAudioPlayback();
	static void configFrameTime();
	static void clearInputBuffers(EmuInputView &view);
	static void handleInputAction(uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static uint translateInputAction(uint input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool touchControlsApplicable();
	static bool handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect);
	static void stopSound();
	static void startSound();
	static void writeSound(const void *samples, uint framesToWrite);
	static uint advanceFramesWithTime(Base::FrameTimeBase time);
	static void setupGamePaths(const char *filePath);
	static void setGameSavePath(const char *path);
	static void setupGameSavePath();
	static void clearGamePaths();
	static FS::PathString baseDefaultGameSavePath();
	static IG::Time benchmark();
	static bool gameIsRunning()
	{
		return !string_equal(gameName_.data(), "");
	}
	static void resetFrameTime();
	static void prepareAudioVideo();
	static void pause();
	static void start();
	static void closeSystem();
	static void closeGame(bool allowAutosaveState = 1);
	[[gnu::format(printf, 1, 2)]]
	static Error makeError(const char *msg, ...);
	static Error makeBlankError();
};

static const char *stateNameStr(int slot)
{
	assert(slot >= -1 && slot < 10);
	static const char *str[] = { "Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	return str[slot+1];
}

#if defined CONFIG_INPUT_POINTING_DEVICES
#define CONFIG_EMUFRAMEWORK_VCONTROLS
#endif

#if defined CONFIG_INPUT_KEYBOARD_DEVICES
#define CONFIG_INPUT_ICADE
#endif
