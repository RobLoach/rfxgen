/*******************************************************************************************
*
*   rFXGen v3.0 - A simple and easy to use sounds generator (based on Tomas Petterson sfxr)
*
*   CONFIGURATION:
*
*   #define CUSTOM_MODAL_DIALOGS
*       Use custom raygui generated modal dialogs instead of native OS ones
*       NOTE: Avoids including tinyfiledialogs depencency library
*
*   VERSIONS HISTORY:
*       3.0  (30-Sep-2022) Updated to raylib 4.2 and raygui 3.2
*                          UI redesigned to follow raylibtech UI conventions
*                          Added main toolbar to access File/Tools/Visual options
*                          Added help window with keboard shortcuts info
*                          Added one extra sound slot and key selection
*                          Removed support for .sfs files (import issues)
*                          Fixed issues when exporting wave to code file
*                          Added a new gui style: terminal
*       2.5  (28-Dec-2021) Updated to raylib 4.2-dev and raygui 3.1
*                          Fixed issue with 32bit float WAV export
*                          Fixed issue with WaveMutate() convergence
*                          Removed tool references to ZERO or ONE
*                          Reviewed code naming conventions
*                          Added a new gui style: lavanda
*       2.3  (20-Dec-2020) Updated to raylib 3.5
*       2.2  (23-Feb-2019) Updated to raylib 3.0, raygui 2.7 and adapted for web
*       2.1  (09-Sep-2019) Ported to latest raygui 2.6
*                          Support custom file dialogs (on non-DESKTOP platforms)
*                          Slight screen resize to adapt to new styles fonts
*       2.0  (xx-Nov-2018) GUI redesigned, CLI improvements
*       1.8  (10-Oct-2018) Functions renaming, code reorganized, better consistency...
*       1.5  (23-Sep-2018) Support .wav export to code and sound playing on command line
*       1.4  (15-Sep-2018) Redesigned command line and comments
*       1.3  (15-May-2018) Reimplemented gui using rGuiLayout
*       1.2  (16-Mar-2018) Working on some code improvements and GUI review
*       1.1  (01-Oct-2017) Code review, simplified
*       1.0  (18-Mar-2017) First release
*       0.9x (XX-Jan-2017) Review complete file...
*       0.95 (14-Sep-2016) Reviewed comments and .rfx format
*       0.9  (12-Sep-2016) Defined WaveParams struct and command line functionality
*       0.8  (09-Sep-2016) Added open/save file dialogs using tinyfiledialogs library
*       0.7  (04-Sep-2016) Program variables renaming for consistency, code reorganized
*       0.6  (30-Aug-2016) Interface redesigned (reduced size) and new features added (wave drawing)
*       0.5  (27-Aug-2016) Completed port and adaptation from sfxr (only sound generation and playing)
*
*   DEPENDENCIES:
*       raylib 4.2              - Windowing/input management and drawing
*       raygui 3.2              - Immediate-mode GUI controls with custom styling and icons
*       tinyfiledialogs 3.8.8   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs
*
*   COMPILATION (Windows - MinGW):
*       gcc -o rfxgen.exe rfxgen.c external/tinyfiledialogs.c -s rfxgen_icon -Iexternal /
*           -lraylib -lopengl32 -lgdi32 -lcomdlg32 -lole32 -std=c99 -Wl,--subsystem,windows
*
*   COMPILATION (Linux - GCC):
*       gcc -o rfxgen rfxgen.c external/tinyfiledialogs.c -s -Iexternal -no-pie -D_DEFAULT_SOURCE /
*           -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
*
*   NOTE: On PLATFORM_ANDROID and PLATFORM_WEB file dialogs are not available
*
*   DEVELOPERS:
*       Ramon Santamaria (@raysan5):   Developer, supervisor, updater and maintainer.
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2022 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#define TOOL_NAME               "rFXGen"
#define TOOL_SHORT_NAME         "rFX"
#define TOOL_VERSION            "3.0"
#define TOOL_DESCRIPTION        "A simple and easy-to-use fx sounds generator"
#define TOOL_RELEASE_DATE       "Oct.2022"
#define TOOL_LOGO_COLOR         0x5197d4ff

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS        // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>  // Emscripten library - LLVM to JavaScript compiler
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                     // Required for: IMGUI controls

#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again

#define GUI_WINDOW_ABOUT_IMPLEMENTATION
#include "gui_window_about.h"           // GUI: About Window

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"           // GUI: File Dialogs

#define GUI_MAIN_TOOLBAR_IMPLEMENTATION
#include "gui_main_toolbar.h"           // GUI: Main toolbar

// raygui embedded styles
#include "styles/style_jungle.h"        // raygui style: jungle
#include "styles/style_candy.h"         // raygui style: candy
#include "styles/style_lavanda.h"       // raygui style: lavanda
#include "styles/style_cyber.h"         // raygui style: cyber
#include "styles/style_terminal.h"      // raygui style: terminal

#include <math.h>                       // Required for: sinf(), powf()
#include <time.h>                       // Required for: clock()
#include <stdlib.h>                     // Required for: calloc(), free()
#include <string.h>                     // Required for: strcmp()
#include <stdio.h>                      // Required for: FILE, fopen(), fread(), fwrite(), ftell(), fseek() fclose()
                                        // NOTE: Used on functions: LoadSound(), SaveSound(), WriteWAV()
#if defined(_WIN32)
    #include <conio.h>                  // Required for: kbhit() [Windows only, no stardard library]
#else
    // Provide kbhit() function in non-Windows platforms
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
bool __stdcall FreeConsole(void);       // Close console from code (kernel32.lib)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
  #define LOG(...) printf(__VA_ARGS__)
#else
  #define LOG(...)
#endif

#define MAX_WAVE_SLOTS       5          // Number of wave slots for generation

// Float random number generation
#define frnd(range) ((float)GetRandomValue(0, 10000)/10000.0f*range)

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Wave parameters type (96 bytes)
typedef struct WaveParams {

    // Random seed used to generate the wave
    int randSeed;

    // Wave type (square, sawtooth, sine, noise)
    int waveTypeValue;

    // Wave envelope parameters
    float attackTimeValue;
    float sustainTimeValue;
    float sustainPunchValue;
    float decayTimeValue;

    // Frequency parameters
    float startFrequencyValue;
    float minFrequencyValue;
    float slideValue;
    float deltaSlideValue;
    float vibratoDepthValue;
    float vibratoSpeedValue;
    //float vibratoPhaseDelayValue;

    // Tone change parameters
    float changeAmountValue;
    float changeSpeedValue;

    // Square wave parameters
    float squareDutyValue;
    float dutySweepValue;

    // Repeat parameters
    float repeatSpeedValue;

    // Phaser parameters
    float phaserOffsetValue;
    float phaserSweepValue;

    // Filter parameters
    float lpfCutoffValue;
    float lpfCutoffSweepValue;
    float lpfResonanceValue;
    float hpfCutoffValue;
    float hpfCutoffSweepValue;

} WaveParams;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *toolName = TOOL_NAME;
static const char *toolVersion = TOOL_VERSION;
static const char *toolDescription = TOOL_DESCRIPTION;

#define HELP_LINES_COUNT    16

// Tool help info
static const char *helpLines[HELP_LINES_COUNT] = {
    "F1 - Show Help window",
    "F2 - Show About window",
    "F3 - Show User window",
    "LCTRL + N - Reset sound slot",
    "LCTRL + O - Open sound file (.rfx)",
    "LCTRL + S - Save sound file (.rfx)",
    "LCTRL + E - Export wave file",
    "-Tool Controls",
    "1-2-3-4-5 - Select current sound slot",
    "SPACE - Play current sound slot",
    "P - Toggle autoplay on params change",
    "-Tool Visuals",
    "LEFT | RIGHT - Select visual style",
    "F - Toggle double screen size",
    NULL,
    "ESCAPE - Close Window/Exit"
};

static float volumeValue = 0.6f;        // Master volume

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

// Load/Save/Export data functions
static WaveParams LoadWaveParams(const char *fileName);                 // Load wave parameters from file
static void SaveWaveParams(WaveParams params, const char *fileName);    // Save wave parameters to file
static void ResetWaveParams(WaveParams *params);                        // Reset wave parameters
static Wave GenerateWave(WaveParams params);                            // Generate wave data from parameters

// Sound generation functions
static WaveParams GenPickupCoin(void);      // Generate sound: Pickup/Coin
static WaveParams GenLaserShoot(void);      // Generate sound: Laser shoot
static WaveParams GenExplosion(void);       // Generate sound: Explosion
static WaveParams GenPowerup(void);         // Generate sound: Powerup
static WaveParams GenHitHurt(void);         // Generate sound: Hit/Hurt
static WaveParams GenJump(void);            // Generate sound: Jump
static WaveParams GenBlipSelect(void);      // Generate sound: Blip/Select
static WaveParams GenRandomize(void);       // Generate random sound
static void WaveMutate(WaveParams *params); // Mutate current sound

// Auxiliar functions
static void DrawWave(Wave *wave, Rectangle bounds, Color color);    // Draw wave data using lines
static int GuiHelpWindow(Rectangle bounds, const char *title, const char **helpLines, int helpLinesCount); // Draw help window with the provided lines

#if defined(PLATFORM_DESKTOP)
static void WaitTimePlayer(int ms);             // Simple time wait in milliseconds for the CLI player
static void PlayWaveCLI(Wave wave);             // Play provided wave through CLI
#if !defined(_WIN32)
static int kbhit(void);                         // Check if a key has been pressed
static char getch(void) { return getchar(); }   // Get pressed character
#endif
#endif  // PLATFORM_DESKTOP

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    char inFileName[512] = { 0 };       // Input file name (required in case of drag & drop over executable)
    char outFileName[512] = { 0 };      // Output file name (required for file save/export)

#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif
#if defined(PLATFORM_DESKTOP)
    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        if ((argc == 2) &&
            (strcmp(argv[1], "-h") != 0) &&
            (strcmp(argv[1], "--help") != 0))       // One argument (file dropped over executable?)
        {
            if (IsFileExtension(argv[1], ".rfx"))   // || IsFileExtension(argv[1], ".sfs"))
            {
                strcpy(inFileName, argv[1]);        // Read input filename to open with gui interface
            }
        }
        else
        {
            ProcessCommandLine(argc, argv);
            return 0;
        }
    }
#endif  // PLATFORM_DESKTOP
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
    // WARNING (Windows): If program is compiled as Window application (instead of console),
    // no console is available to show output info... solution is compiling a console application
    // and closing console (FreeConsole()) when changing to GUI interface
    FreeConsole();
#endif

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 540;
    const int screenHeight = 580;

    //SetConfigFlags(FLAG_MSAA_4X_HINT);        // Window configuration flags
    InitWindow(screenWidth, screenHeight, TextFormat("%s v%s | %s", toolName, toolVersion, toolDescription));
    SetExitKey(0);

    InitAudioDevice();

    // GUI: Main Layout
    //-----------------------------------------------------------------------------------
    bool playOnChange = true;           // Automatically play sound on parameter change
    bool screenSizeActive = false;      // Scale screen x2 (useful for HighDPI screens)

    bool helpWindowActive = false;      // Show window: help info 
    bool userWindowActive = false;      // Show window: user registration
    //-----------------------------------------------------------------------------------

    // GUI: About Window
    //-----------------------------------------------------------------------------------
    GuiWindowAboutState windowAboutState = InitGuiWindowAbout();
    //-----------------------------------------------------------------------------------

    // GUI: Main toolbar panel (file and visualization)
    //-----------------------------------------------------------------------------------
    GuiMainToolbarState mainToolbarState = InitGuiMainToolbar();
    //-----------------------------------------------------------------------------------

    // GUI: Export Window
    //-----------------------------------------------------------------------------------
    bool exportWindowActive = false;

    int fileTypeActive = 0;         // ComboBox file type selection
    int sampleRateActive = 1;       // ComboBox sample rate selection 
    int sampleSizeActive = 1;       // ComboBox sample size selection
    int channelsActive = 0;         // ComboBox channels selection

    int exportSampleSize = 32;      // Export wave sample size in bits (bitrate)
    int exportSampleRate = 44100;   // Export wave sample rate (frequency)
    int exportChannels = 1;         // Export wave channels
    //-----------------------------------------------------------------------------------

    // GUI: Exit Window
    //-----------------------------------------------------------------------------------
    bool closeWindow = false;
    bool exitWindowActive = false;
    //-----------------------------------------------------------------------------------

    // GUI: Custom file dialogs
    //-----------------------------------------------------------------------------------
    bool showLoadFileDialog = false;
    bool showSaveFileDialog = false;
    bool showExportFileDialog = false;
    //-----------------------------------------------------------------------------------

    // Wave and Sound Initialization
    //-----------------------------------------------------------------------------------
    WaveParams params[MAX_WAVE_SLOTS] = { 0 }; // Wave parameters for generation
    Wave wave[MAX_WAVE_SLOTS] = { 0 };
    Sound sound[MAX_WAVE_SLOTS] = { 0 };

    for (int i = 0; i < MAX_WAVE_SLOTS; i++)
    {
        // Reset generation parameters
        // NOTE: Random seed for generation is set
        ResetWaveParams(&params[i]);

        // Default wave values
        wave[i].sampleRate = 44100;
        wave[i].sampleSize = 32;        // 32 bit -> float
        wave[i].channels = 1;           // 1 channel -> mono
        wave[i].frameCount = 10*wave[i].sampleRate;    // Max frame count for 10 seconds
        wave[i].data = (float *)RL_CALLOC(wave[i].frameCount, sizeof(float));

        sound[i] = LoadSoundFromWave(wave[i]);
    }
    //-----------------------------------------------------------------------------------

    // Check if a wave parameters file has been provided on command line
    if (inFileName[0] != '\0')
    {
        // Clean everything (just in case)
        UnloadWave(wave[0]);
        UnloadSound(sound[0]);

        params[0] = LoadWaveParams(inFileName); // Load wave parameters from .rfx
        wave[0] = GenerateWave(params[0]);      // Generate wave from parameters
        sound[0] = LoadSoundFromWave(wave[0]);  // Load sound from new wave

        PlaySound(sound[0]);                    // Play generated sound
    }

    bool regenerate = false;                    // Wave regeneration required
    
    float prevVolumeValue = volumeValue;
    int prevWaveTypeValue[MAX_WAVE_SLOTS] = { params[0].waveTypeValue };
    
    Rectangle waveRec = { 12, 484, 516, 64 };       // Wave drawing rectangle box
    Rectangle slidersRec = { 256, 82, 226, 392 };   // Area defining sliders to allow sound replay when mouse-released

    // Set default sound volume
    for (int i = 0; i < MAX_WAVE_SLOTS; i++) SetSoundVolume(sound[i], volumeValue);

    // Render texture to draw wave at x2, it will be scaled down with bilinear filtering (cheapre than MSAA x4)
    RenderTexture2D waveTarget = LoadRenderTexture((int)waveRec.width*2, (int)waveRec.height*2);
    SetTextureFilter(waveTarget.texture, TEXTURE_FILTER_BILINEAR);

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    RenderTexture2D screenTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!closeWindow)    // Detect window close button
    {
        // WARNING: ASINCIFY requires this line, 
        // it contains the call to emscripten_sleep() for PLATFORM_WEB
        if (WindowShouldClose()) closeWindow = true;

        // Dropped files logic
        //----------------------------------------------------------------------------------
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            // Support loading .rfx files (wave parameters)
            if (IsFileExtension(droppedFiles.paths[0], ".rfx")) // || IsFileExtension(droppedFiles.paths[0], ".sfs"))
            {
                params[mainToolbarState.soundSlotActive] = LoadWaveParams(droppedFiles.paths[0]);
                regenerate = true;

                SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(droppedFiles.paths[0])));
            }
            else if (IsFileExtension(droppedFiles.paths[0], ".rgs")) GuiLoadStyle(droppedFiles.paths[0]);

            UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }
        //----------------------------------------------------------------------------------

        // Keyboard shortcuts
        //------------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) PlaySound(sound[mainToolbarState.soundSlotActive]);  // Play current sound

        // Show dialog: save sound (.rfx)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) showSaveFileDialog = true;

        // Show dialog: load sound (.rfx)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) showLoadFileDialog = true;

        // Show dialog: export wave (.wav, .raw, .h)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) exportWindowActive = true;

        // Select current sound slot
        if (IsKeyPressed(KEY_ONE)) mainToolbarState.soundSlotActive = 0;
        else if (IsKeyPressed(KEY_TWO)) mainToolbarState.soundSlotActive = 1;
        else if (IsKeyPressed(KEY_THREE)) mainToolbarState.soundSlotActive = 2;
        else if (IsKeyPressed(KEY_FOUR)) mainToolbarState.soundSlotActive = 3;
        else if (IsKeyPressed(KEY_FIVE)) mainToolbarState.soundSlotActive = 4;

        // Select visual style
        if (IsKeyPressed(KEY_LEFT)) mainToolbarState.visualStyleActive--;
        else if (IsKeyPressed(KEY_RIGHT)) mainToolbarState.visualStyleActive++;
        if (mainToolbarState.visualStyleActive < 0) mainToolbarState.visualStyleActive = 5;
        else if (mainToolbarState.visualStyleActive > 5) mainToolbarState.visualStyleActive = 0;

#if !defined(PLATFORM_WEB)
        // Toggle screen size (x2) mode
        if (IsKeyPressed(KEY_F)) screenSizeActive = !screenSizeActive;
#endif
        // Toggle play on change option
        if (IsKeyPressed(KEY_P)) playOnChange = !playOnChange;

        // Toggle window help
        if (IsKeyPressed(KEY_F1)) helpWindowActive = !helpWindowActive;

        // Toggle window about
        if (IsKeyPressed(KEY_F2)) windowAboutState.windowActive = !windowAboutState.windowActive;

        // Toggle window registered user
        //if (IsKeyPressed(KEY_F3)) userWindowActive = !userWindowActive;

        // Show closing window on ESC
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (windowAboutState.windowActive) windowAboutState.windowActive = false;
            else if (helpWindowActive) helpWindowActive = false;
            else if (exportWindowActive) exportWindowActive = false;
        #if !defined(PLATFORM_WEB)
            else exitWindowActive = !exitWindowActive;
        #else
            else if (showLoadFileDialog) showLoadFileDialog = false;
            else if (showSaveFileDialog) showSaveFileDialog = false;
            else if (showExportFileDialog) showExportFileDialog = false;
        #endif
        }
        //----------------------------------------------------------------------------------

        // Main toolbar logic
        //----------------------------------------------------------------------------------
        // File options logic
        if (mainToolbarState.btnNewFilePressed)
        {
            // Reload current slot
            UnloadSound(sound[mainToolbarState.soundSlotActive]);
            UnloadWave(wave[mainToolbarState.soundSlotActive]);
            
            wave[mainToolbarState.soundSlotActive].data = (float *)RL_CALLOC(wave[mainToolbarState.soundSlotActive].frameCount, sizeof(float));
            sound[mainToolbarState.soundSlotActive] = LoadSoundFromWave(wave[mainToolbarState.soundSlotActive]);
        }
        else if (mainToolbarState.btnLoadFilePressed) showLoadFileDialog = true;
        else if (mainToolbarState.btnSaveFilePressed) showSaveFileDialog = true;
        else if (mainToolbarState.btnExportFilePressed) exportWindowActive = true;

        if (mainToolbarState.visualStyleActive != mainToolbarState.prevVisualStyleActive)
        {
            GuiLoadStyleDefault();

            switch (mainToolbarState.visualStyleActive)
            {
                case 1: GuiLoadStyleJungle(); break;
                case 2: GuiLoadStyleCandy(); break;
                case 3: GuiLoadStyleLavanda(); break;
                case 4: GuiLoadStyleCyber(); break;
                case 5: GuiLoadStyleTerminal(); break;
                default: break;
            }

            GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

            mainToolbarState.prevVisualStyleActive = mainToolbarState.visualStyleActive;
        }

        // Help options logic
        if (mainToolbarState.btnHelpPressed) helpWindowActive = true;               // Help button logic
        if (mainToolbarState.btnAboutPressed) windowAboutState.windowActive = true; // About window button logic
        if (mainToolbarState.btnUserPressed) userWindowActive = true;               // User button logic
        //----------------------------------------------------------------------------------

        // Basic program flow logic
        //----------------------------------------------------------------------------------
        // Check for changed gui values
        if (volumeValue != prevVolumeValue)
        {
            SetMasterVolume(volumeValue);
            prevVolumeValue = volumeValue;
        }

        // Check wave type combobox selection to regenerate wave
        if (params[mainToolbarState.soundSlotActive].waveTypeValue != prevWaveTypeValue[mainToolbarState.soundSlotActive]) regenerate = true;
        prevWaveTypeValue[mainToolbarState.soundSlotActive] = params[mainToolbarState.soundSlotActive].waveTypeValue;
        
        // Avoid wave regeneration when some window is active
        if (!windowAboutState.windowActive && 
            !helpWindowActive && 
            !showLoadFileDialog &&
            !showSaveFileDialog &&
            !showExportFileDialog &&
            !exportWindowActive && 
            !exitWindowActive)
        {
            // Consider two possible cases to regenerate wave and update sound:
            // CASE1: regenerate flag is true (set by sound buttons functions)
            // CASE2: Mouse is moving sliders and mouse is released (checks against slidersRec)
            if (regenerate || ((CheckCollisionPointRec(GetMousePosition(), slidersRec)) && (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))))
            {
                UnloadWave(wave[mainToolbarState.soundSlotActive]);
                UnloadSound(sound[mainToolbarState.soundSlotActive]);
                
                wave[mainToolbarState.soundSlotActive] = GenerateWave(params[mainToolbarState.soundSlotActive]);        // Generate new wave from parameters
                sound[mainToolbarState.soundSlotActive] = LoadSoundFromWave(wave[mainToolbarState.soundSlotActive]);    // Reload sound from new wave

                if ((regenerate || playOnChange) && !GuiIsLocked()) PlaySound(sound[mainToolbarState.soundSlotActive]);

                regenerate = false;
            }
        }

        // Check slot change to play next one selected
        if (mainToolbarState.soundSlotActive != mainToolbarState.prevSoundSlotActive)
        {
            PlaySound(sound[mainToolbarState.soundSlotActive]);
            mainToolbarState.prevSoundSlotActive = mainToolbarState.soundSlotActive;
        }

        // Screen scale logic (x2)
        //----------------------------------------------------------------------------------
        if (screenSizeActive)
        {
            // Screen size x2
            if (GetScreenWidth() < screenWidth*2)
            {
                SetWindowSize(screenWidth*2, screenHeight*2);
                SetMouseScale(0.5f, 0.5f);
            }
        }
        else
        {
            // Screen size x1
            if (screenWidth*2 >= GetScreenWidth())
            {
                SetWindowSize(screenWidth, screenHeight);
                SetMouseScale(1.0f, 1.0f);
            }
        }
        
        // WARNING: Some windows should lock the main screen controls when shown
        if (windowAboutState.windowActive || 
            helpWindowActive ||
            userWindowActive ||
            exitWindowActive || 
            exportWindowActive ||
            showLoadFileDialog || 
            showSaveFileDialog || 
            showExportFileDialog) GuiLock();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Render wave data to texture
        BeginTextureMode(waveTarget);
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            DrawWave(&wave[mainToolbarState.soundSlotActive], (Rectangle){ 0, 0, (float)waveTarget.texture.width, (float)waveTarget.texture.height }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_PRESSED)));
        EndTextureMode();

        // Render all screen to texture (for scaling)
        BeginTextureMode(screenTarget);
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            // GUI: Main toolbar panel
            //----------------------------------------------------------------------------------
            GuiMainToolbar(&mainToolbarState);
            //----------------------------------------------------------------------------------

            // rFXGen Layout: controls drawing
            //----------------------------------------------------------------------------------
            // Draw left buttons
            int prevTextPadding = GuiGetStyle(BUTTON, TEXT_PADDING);
            GuiSetStyle(BUTTON, TEXT_PADDING, 3);
            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            if (GuiButton((Rectangle){ 12, 48, 108, 24 }, "#131#Play Sound")) PlaySound(sound[mainToolbarState.soundSlotActive]);

            if (GuiButton((Rectangle){ 12, 88, 108, 24 }, "#146#Pickup/Coin")) { params[mainToolbarState.soundSlotActive] = GenPickupCoin(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 116, 108, 24 }, "#145#Laser/Shoot")) { params[mainToolbarState.soundSlotActive] = GenLaserShoot(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 144, 108, 24 }, "#147#Explosion")) { params[mainToolbarState.soundSlotActive] = GenExplosion(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 172, 108, 24 }, "#148#PowerUp")) { params[mainToolbarState.soundSlotActive] = GenPowerup(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 200, 108, 24 }, "#152#Hit/Hurt")) { params[mainToolbarState.soundSlotActive] = GenHitHurt(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 228, 108, 24 }, "#150#Jump")) { params[mainToolbarState.soundSlotActive] = GenJump(); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 256, 108, 24 }, "#144#Blip/Select")) { params[mainToolbarState.soundSlotActive] = GenBlipSelect(); regenerate = true; }
            GuiSetStyle(BUTTON, TEXT_PADDING, prevTextPadding);
            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

            GuiLine((Rectangle){ 12, 248 + 32, 108, 16 }, NULL);

            GuiSetStyle(TOGGLE, TEXT_PADDING, 3);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            params[mainToolbarState.soundSlotActive].waveTypeValue = GuiToggleGroup((Rectangle){ 12, 248 + 32 + 16, 108, 24 }, "#126#Square\n#127#Sawtooth\n#125#Sinewave\n#124#Noise", params[mainToolbarState.soundSlotActive].waveTypeValue);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 0);

            GuiLine((Rectangle){ 12, 248 + 32 + 16 + 4*24 + 3*2, 108, 16 }, NULL);

            if (GuiButton((Rectangle){ 12, 414, 108, 24 }, "#75#Mutate")) { WaveMutate(&params[mainToolbarState.soundSlotActive]); regenerate = true; }
            if (GuiButton((Rectangle){ 12, 414 + 24 + 4, 108, 24 }, "#77#Randomize")) { params[mainToolbarState.soundSlotActive] = GenRandomize(); regenerate = true; }

            // Parameters sliders
            //--------------------------------------------------------------------------------
            Vector2 paramsPos = { 260, 56 };

            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y - 8, 398, 24 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 24, 398, 16*4 + 8 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 95, 398, 16*2 + 8 + 1 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 135, 398, 16*4 + 8 + 2 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 208, 398, 16*4 + 8 + 1 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 280, 398, 16*3 + 8 }, NULL);
            GuiGroupBox((Rectangle){ paramsPos.x - 130, paramsPos.y + 335, 398, 16*5 + 8 + 1 }, NULL);

            volumeValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y - 2, 220, 12 }, "VOLUME", TextFormat("%i", (int)(volumeValue*100)), volumeValue, 0, 1);

            params[mainToolbarState.soundSlotActive].attackTimeValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 30, 220, 12 }, "ATTACK TIME", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].attackTimeValue), params[mainToolbarState.soundSlotActive].attackTimeValue, 0, 1);
            params[mainToolbarState.soundSlotActive].sustainTimeValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "SUSTAIN TIME", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].sustainTimeValue), params[mainToolbarState.soundSlotActive].sustainTimeValue, 0, 1);
            params[mainToolbarState.soundSlotActive].sustainPunchValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "SUSTAIN PUNCH", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].sustainPunchValue), params[mainToolbarState.soundSlotActive].sustainPunchValue, 0, 1);
            params[mainToolbarState.soundSlotActive].decayTimeValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "DECAY TIME", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].decayTimeValue), params[mainToolbarState.soundSlotActive].decayTimeValue, 0, 1);
            
            params[mainToolbarState.soundSlotActive].startFrequencyValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 24, 220, 12 }, "START FREQUENCY", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].startFrequencyValue), params[mainToolbarState.soundSlotActive].startFrequencyValue, 0, 1);
            params[mainToolbarState.soundSlotActive].minFrequencyValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "MIN FREQUENCY", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].minFrequencyValue), params[mainToolbarState.soundSlotActive].minFrequencyValue, 0, 1);
            
            params[mainToolbarState.soundSlotActive].slideValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 24, 220, 12 }, "SLIDE", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].slideValue), params[mainToolbarState.soundSlotActive].slideValue, -1, 1);
            params[mainToolbarState.soundSlotActive].deltaSlideValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "DELTA SLIDE", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].deltaSlideValue), params[mainToolbarState.soundSlotActive].deltaSlideValue, -1, 1);
            params[mainToolbarState.soundSlotActive].vibratoDepthValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "VIBRATO DEPTH", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].vibratoDepthValue), params[mainToolbarState.soundSlotActive].vibratoDepthValue, 0, 1);
            params[mainToolbarState.soundSlotActive].vibratoSpeedValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "VIBRATO SPEED", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].vibratoSpeedValue), params[mainToolbarState.soundSlotActive].vibratoSpeedValue, 0, 1);
            
            params[mainToolbarState.soundSlotActive].changeAmountValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 24, 220, 12 }, "CHANGE AMOUNT", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].changeAmountValue), params[mainToolbarState.soundSlotActive].changeAmountValue, -1, 1);
            params[mainToolbarState.soundSlotActive].changeSpeedValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "CHANGE SPEED", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].changeSpeedValue), params[mainToolbarState.soundSlotActive].changeSpeedValue, 0, 1);
            params[mainToolbarState.soundSlotActive].squareDutyValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "SQUARE DUTY", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].squareDutyValue), params[mainToolbarState.soundSlotActive].squareDutyValue, 0, 1);
            params[mainToolbarState.soundSlotActive].dutySweepValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "DUTY SWEEP", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].dutySweepValue), params[mainToolbarState.soundSlotActive].dutySweepValue, -1, 1);
           
            params[mainToolbarState.soundSlotActive].repeatSpeedValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 24, 220, 12 }, "REPEAT SPEED", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].repeatSpeedValue), params[mainToolbarState.soundSlotActive].repeatSpeedValue, 0, 1);
            params[mainToolbarState.soundSlotActive].phaserOffsetValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "PHASER OFFSET", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].phaserOffsetValue), params[mainToolbarState.soundSlotActive].phaserOffsetValue, -1, 1);
            params[mainToolbarState.soundSlotActive].phaserSweepValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "PHASER SWEEP", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].phaserSweepValue), params[mainToolbarState.soundSlotActive].phaserSweepValue, -1, 1);
            
            params[mainToolbarState.soundSlotActive].lpfCutoffValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 24, 220, 12 }, "LPF CUTOFF", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].lpfCutoffValue), params[mainToolbarState.soundSlotActive].lpfCutoffValue, 0, 1);
            params[mainToolbarState.soundSlotActive].lpfCutoffSweepValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "LPF CUTOFF SWEEP", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].lpfCutoffSweepValue), params[mainToolbarState.soundSlotActive].lpfCutoffSweepValue, -1, 1);
            params[mainToolbarState.soundSlotActive].lpfResonanceValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "LPF RESONANCE", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].lpfResonanceValue), params[mainToolbarState.soundSlotActive].lpfResonanceValue, 0, 1);
            params[mainToolbarState.soundSlotActive].hpfCutoffValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "HPF CUTOFF", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].hpfCutoffValue), params[mainToolbarState.soundSlotActive].hpfCutoffValue, 0, 1);
            params[mainToolbarState.soundSlotActive].hpfCutoffSweepValue = GuiSliderBar((Rectangle){ paramsPos.x, paramsPos.y += 16, 220, 12 }, "HPF CUTOFF SWEEP", TextFormat("%.2f", params[mainToolbarState.soundSlotActive].hpfCutoffSweepValue), params[mainToolbarState.soundSlotActive].hpfCutoffSweepValue, -1, 1);
            //--------------------------------------------------------------------------------

            // Draw Wave form
            //--------------------------------------------------------------------------------
            DrawTextureEx(waveTarget.texture, (Vector2) { waveRec.x, waveRec.y }, 0.0f, 0.5f, WHITE);
            DrawRectangle((int)waveRec.x, (int)waveRec.y + (int)waveRec.height/2, (int)waveRec.width, 1, Fade(GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_FOCUSED)), 0.6f));
            DrawRectangleLines((int)waveRec.x, (int)waveRec.y, (int)waveRec.width, (int)waveRec.height, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
            //--------------------------------------------------------------------------------

            // GUI: Status bar
            //----------------------------------------------------------------------------------
            int textPadding = GuiGetStyle(STATUSBAR, TEXT_PADDING);
            GuiSetStyle(STATUSBAR, TEXT_PADDING, 0);
            GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            GuiStatusBar((Rectangle){ 0, screenHeight - 24, 190, 24 }, TextFormat("Total Frames: %i", wave[mainToolbarState.soundSlotActive].frameCount));
            GuiStatusBar((Rectangle){ 190 - 1, screenHeight - 24, 170, 24 }, TextFormat("Duration: %i ms", wave[mainToolbarState.soundSlotActive].frameCount*1000/(wave[mainToolbarState.soundSlotActive].sampleRate)));
            GuiStatusBar((Rectangle){ 190 + 170 - 2, screenHeight - 24, screenWidth - (190 + 170 - 2), 24 }, TextFormat("Size: %i bytes", wave[mainToolbarState.soundSlotActive].frameCount *wave[mainToolbarState.soundSlotActive].channels *exportSampleSize/8));
            GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            GuiSetStyle(STATUSBAR, TEXT_PADDING, textPadding);
            //----------------------------------------------------------------------------------

            // NOTE: If some overlap window is open and main window is locked, we draw a background rectangle
            if (GuiIsLocked()) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));
            
            // WARNING: Before drawing the windows, we unlock them
            GuiUnlock();

            // GUI: About Window
            //--------------------------------------------------------------------------------
            GuiWindowAbout(&windowAboutState);
            //--------------------------------------------------------------------------------

            // GUI: Help Window
            //----------------------------------------------------------------------------------------
            Rectangle helpWindowBounds = { (float)screenWidth/2 - 330/2, (float)screenHeight/2 - 400.0f/2, 330, 0 };
            if (helpWindowActive) helpWindowActive = GuiHelpWindow(helpWindowBounds, GuiIconText(ICON_HELP, TextFormat("%s Shortcuts", TOOL_NAME)), helpLines, HELP_LINES_COUNT);
            //----------------------------------------------------------------------------------------

            // GUI: Export Window
            //----------------------------------------------------------------------------------------
            if (exportWindowActive)
            {
                Rectangle messageBox = { (float)screenWidth/2 - 248/2, (float)screenHeight/2 - 150, 248, 208 };
                int result = GuiMessageBox(messageBox, "#7#Export Wave File", " ", "#7# Export Wave");

                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12, 106, 24 }, "File Format:");
                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12 + 24 + 8, 106, 24 }, "Sample Rate:");
                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12 + 48 + 16, 106, 24 }, "Sample Size:");
                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12 + 72 + 24, 106, 24 }, "Channels:");
                
                fileTypeActive = GuiComboBox((Rectangle) { messageBox.x + 12 + 100, messageBox.y + 24 + 12, 124, 24 }, "WAV;RAW;CODE", fileTypeActive);
                sampleRateActive = GuiComboBox((Rectangle) { messageBox.x + 12 + 100, messageBox.y + 24 + 12 + 24 + 8, 124, 24 }, "22050 Hz;44100 Hz", sampleRateActive);
                sampleSizeActive = GuiComboBox((Rectangle) { messageBox.x + 12 + 100, messageBox.y + 24 + 12 + 48 + 16, 124, 24 }, "8 bit;16 bit;32 bit", sampleSizeActive);
                channelsActive = GuiComboBox((Rectangle){ messageBox.x + 12 + 100, messageBox.y + 24 + 12 + 72 + 24, 124, 24 }, "Mono;Stereo", channelsActive);

                if (result == 1)    // Export button pressed
                {
                    // Update export option from combobox selections
                    if (sampleRateActive == 0) exportSampleRate = 22050;
                    else if (sampleRateActive == 1) exportSampleRate = 44100;

                    if (sampleSizeActive == 0) exportSampleSize = 8;
                    else if (sampleSizeActive == 1) exportSampleSize = 16;
                    else if (sampleSizeActive == 2) exportSampleSize = 32;

                    exportChannels = channelsActive + 1;

                    exportWindowActive = false;
                    showExportFileDialog = true;
                }
                else if (result == 0) exportWindowActive = false;
            }
            //----------------------------------------------------------------------------------

            // GUI: Exit Window
            //----------------------------------------------------------------------------------------
            if (exitWindowActive)
            {
                int result = GuiMessageBox((Rectangle) { (float)screenWidth/2 - 125, (float)screenHeight/2 - 50, 250, 100 }, "#159#Closing rFXGen", "Do you really want to exit?", "Yes;No");

                if ((result == 0) || (result == 2)) exitWindowActive = false;
                else if (result == 1) closeWindow = true;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load sound file ...", inFileName, "Ok", "Just drag and drop your .rfx sound file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load sound parameters file...", inFileName, "*.rfx", "Sound Param Files (*.rfx)");
#endif
                if (result == 1)
                {
                    // Load parameters file
                    params[mainToolbarState.soundSlotActive] = LoadWaveParams(inFileName);
                    SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                    regenerate = true;
                }

                if (result >= 0) showLoadFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Save File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showSaveFileDialog)
            {
                strcpy(outFileName, "sound.rfx");
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_TEXTINPUT, "Save sound file as...", outFileName, "Ok;Cancel", NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Save sound parameters file...", outFileName, "*.rfx", "Sound Param Files (*.rfx)");
#endif
                if (result == 1)
                {
                    // Save file: outFileName
                    // Check for valid extension and make sure it is
                    if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rfx")) strcat(outFileName, ".rfx\0");
                    SaveWaveParams(params[mainToolbarState.soundSlotActive], outFileName);    // Save wave parameters

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showSaveFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Export File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showExportFileDialog)
            {
                // Consider different supported file types
                char fileTypeFilters[64] = { 0 };
                strcpy(outFileName, "sound");

                if (fileTypeActive == 0) { strcpy(fileTypeFilters, "*.wav"); strcat(outFileName, ".wav"); }
                else if (fileTypeActive == 1) { strcpy(fileTypeFilters, "*.raw"); strcat(outFileName, ".raw"); }
                else if (fileTypeActive == 2) { strcpy(fileTypeFilters, "*.h"); strcat(outFileName, ".h"); }

#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_TEXTINPUT, "Export wave file...", outFileName, "Ok;Cancel", NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Export wave file...", outFileName, fileTypeFilters, TextFormat("File type (%s)", fileTypeFilters));
#endif
                if (result == 1)
                {
                    // Export file: outFileName
                    Wave cwave = WaveCopy(wave[mainToolbarState.soundSlotActive]);
                    WaveFormat(&cwave, exportSampleRate, exportSampleSize, exportChannels);   // Before exporting wave data, we format it as desired

                    if (fileTypeActive == 0) 
                    {
                        // Check for valid extension and make sure it is
                        if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".wav")) strcat(outFileName, ".wav\0");
                        ExportWave(cwave, outFileName);            // Export wave data as WAV file
                    }
                    else if (fileTypeActive == 2)
                    {
                        // Check for valid extension and make sure it is
                        if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".h")) strcat(outFileName, ".h\0");
                        ExportWaveAsCode(cwave, outFileName); // Export wave data as code file
                    }
                    else if (fileTypeActive == 1)   // Export Wave as RAW data
                    {
                        // Check for valid extension and make sure it is
                        if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".raw")) strcat(outFileName, ".raw\0");
                        FILE *rawFile = fopen(outFileName, "wb");

                        if (rawFile != NULL)
                        {
                            fwrite(cwave.data, 1, cwave.frameCount*cwave.channels*cwave.sampleSize/8, rawFile);  // Write wave data
                            fclose(rawFile);
                        }
                    }

                    UnloadWave(cwave);

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showExportFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

        EndTextureMode();

        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            
            // Draw render texture to screen
            if (screenSizeActive) DrawTexturePro(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Rectangle){ 0, 0, (float)screenTarget.texture.width*2, (float)screenTarget.texture.height*2 }, (Vector2){ 0, 0 }, 0.0f, WHITE);
            else DrawTextureRec(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Vector2){ 0, 0 }, WHITE);

        EndDrawing();
        //------------------------------------------------------------------------------------
    }

    // De-Initialization
    //----------------------------------------------------------------------------------------
    for (int i = 0; i < MAX_WAVE_SLOTS; i++)
    {
        UnloadSound(sound[i]);  // Unload sounds
        UnloadWave(wave[i]);    // Unload wave slots (free done internally)
    }

    UnloadRenderTexture(screenTarget);
    UnloadRenderTexture(waveTarget);

    CloseAudioDevice();         // Close audio device
    CloseWindow();              // Close window and OpenGL context
    //----------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
// Show command line usage info
static void ShowCommandLineInfo(void)
{
    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// %s v%s - %s                   //\n", toolName, toolVersion, toolDescription);
    printf("// powered by raylib v%s and raygui v%s                               //\n", RAYLIB_VERSION, RAYGUI_VERSION);
    printf("// more info and bugs-report: github.com/raysan5/rfxgen                         //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2014-2022 raylib technologies (@raylibtech)                    //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("USAGE:\n\n");
    printf("    > rfxgen [--help] --input <filename.ext> [--output <filename.ext>]\n");
    printf("             [--format <sample_rate>,<sample_size>,<channels>] [--play <filename.ext>]\n");

    printf("\nOPTIONS:\n\n");
    printf("    -h, --help                      : Show tool version and command line usage help\n\n");
    printf("    -i, --input <filename.ext>      : Define input file.\n");
    printf("                                      Supported extensions: .rfx, .wav, .ogg, .flac, .mp3\n\n");
    printf("    -o, --output <filename.ext>     : Define output file.\n");
    printf("                                      Supported extensions: .wav, .raw, .h\n");
    printf("                                      NOTE: If not specified, defaults to: output.wav\n\n");
    printf("    -f, --format <sample_rate>,<sample_size>,<channels>\n");
    printf("                                    : Define output wave format. Comma separated values.\n");
    printf("                                      Supported values:\n");
    printf("                                          Sample rate:      22050, 44100\n");
    printf("                                          Sample size:      8, 16, 32\n");
    printf("                                          Channels:         1 (mono), 2 (stereo)\n");
    printf("                                      NOTE: If not specified, defaults to: 44100, 16, 1\n\n");
    printf("    -p, --play <filename.ext>       : Play provided sound.\n");
    printf("                                      Supported extensions: .wav, .ogg, .flac, .mp3\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > rfxgen --input sound.rfx --output jump.wav\n");
    printf("        Process <sound.rfx> to generate <sound.wav> at 44100 Hz, 32 bit, Mono\n\n");
    printf("    > rfxgen --input sound.rfx --output jump.raw --format 22050,16,2\n");
    printf("        Process <sound.rfx> to generate <jump.raw> at 22050 Hz, 16 bit, Stereo\n\n");
    printf("    > rfxgen --input sound.ogg --play output.wav\n");
    printf("        Process <sound.ogg> to generate <output.wav> and play <output.wav>\n\n");
    printf("    > rfxgen --input sound.mp3 --output jump.wav --format 22050,8,1 --play jump.wav\n");
    printf("        Process <sound.mp3> to generate <jump.wav> at 22050 Hz, 8 bit, Stereo.\n");
    printf("        Plays generated sound <jump.wav>.\n");
}

// Process command line input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;         // Toggle command line usage info

    char inFileName[512] = { 0 };       // Input file name
    char outFileName[512] = { 0 };      // Output file name
    char playFileName[512] = { 0 };     // Play file name

    int sampleRate = 44100;             // Default conversion sample rate
    int sampleSize = 16;                // Default conversion sample size
    int channels = 1;                   // Default conversion channels number

    if (argc == 1) showUsageInfo = true;

    // Process command line arguments
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
        }
        else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0))
        {
            // Check for valid argument and valid file extension
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".rfx") ||
                    IsFileExtension(argv[i + 1], ".wav") ||
                    IsFileExtension(argv[i + 1], ".ogg") ||
                    IsFileExtension(argv[i + 1], ".flac") ||
                    IsFileExtension(argv[i + 1], ".mp3"))
                {
                    strcpy(inFileName, argv[i + 1]);    // Read input filename
                }
                else LOG("WARNING: Input file extension not recognized\n");

                i++;
            }
            else LOG("WARNING: No input file provided\n");
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".wav") ||
                    IsFileExtension(argv[i + 1], ".raw") ||
                    IsFileExtension(argv[i + 1], ".h"))
                {
                    strcpy(outFileName, argv[i + 1]);   // Read output filename
                }
                else LOG("WARNING: Output file extension not recognized\n");

                i++;
            }
            else LOG("WARNING: No output file provided\n");
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--format") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                int numValues = 0;
                const char **values = TextSplit(argv[i + 1], ',', &numValues);

                if (numValues != 3) LOG("WARNING: Incorrect number of format values\n");
                else
                {
                    // Read values text and convert to integer values
                    sampleRate = TextToInteger(values[0]);
                    sampleSize = TextToInteger(values[1]);
                    channels = TextToInteger(values[2]);

                    // Verify retrieved values are valid
                    if ((sampleRate != 44100) && (sampleRate != 22050))
                    {
                        LOG("WARNING: Sample rate not supported. Default: 44100 Hz\n");
                        sampleRate = 44100;
                    }

                    if ((sampleSize != 8) && (sampleSize != 16) && (sampleSize != 32))
                    {
                        LOG("WARNING: Sample size not supported. Default: 16 bit\n");
                        sampleSize = 16;
                    }

                    if ((channels != 1) && (channels != 2))
                    {
                        LOG("WARNING: Channels number not supported. Default: 1 (mono)\n");
                        channels = 1;
                    }
                }
            }
            else LOG("WARNING: Format parameters provided not valid\n");
        }
        else if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--play") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".wav") ||
                    IsFileExtension(argv[i + 1], ".ogg") ||
                    IsFileExtension(argv[i + 1], ".flac") ||
                    IsFileExtension(argv[i + 1], ".mp3"))
                {
                    strcpy(playFileName, argv[i + 1]);   // Read filename to play
                    i++;
                }
                else LOG("WARNING: Play file format not supported\n");

                i++;
            }
            else LOG("WARNING: No file to play provided\n");
        }
    }

    // Process input file if provided
    if (inFileName[0] != '\0')
    {
        if (outFileName[0] == '\0') strcpy(outFileName, "output.wav");  // Set a default name for output in case not provided

        LOG("\nInput file:       %s", inFileName);
        LOG("\nOutput file:      %s", outFileName);
        LOG("\nOutput format:    %i Hz, %i bits, %s\n\n", sampleRate, sampleSize, (channels == 1)? "Mono" : "Stereo");

        Wave wave = { 0 };

        if (IsFileExtension(inFileName, ".rfx")) // || IsFileExtension(inFileName, ".sfs"))
        {
            WaveParams params = LoadWaveParams(inFileName);
            wave = GenerateWave(params);
        }
        else if (IsFileExtension(inFileName, ".wav") ||
                 IsFileExtension(inFileName, ".ogg") ||
                 IsFileExtension(inFileName, ".flac") ||
                 IsFileExtension(inFileName, ".mp3"))
        {
            wave = LoadWave(inFileName);
        }

        // Format wave data to desired sampleRate, sampleSize and channels
        WaveFormat(&wave, sampleRate, sampleSize, channels);

        // Export wave data as audio file (.wav) or code file (.h)
        if (IsFileExtension(outFileName, ".wav")) ExportWave(wave, outFileName);
        else if (IsFileExtension(outFileName, ".h")) ExportWaveAsCode(wave, outFileName);
        else if (IsFileExtension(outFileName, ".raw"))
        {
            // Export Wave as RAW data
            FILE *rawFile = fopen(outFileName, "wb");

            if (rawFile != NULL)
            {
                fwrite(wave.data, 1, wave.frameCount*wave.channels*wave.sampleSize/8, rawFile);  // Write wave data
                fclose(rawFile);
            }
        }

        UnloadWave(wave);
    }

    // Play audio file if provided
    if (playFileName[0] != '\0')
    {
        Wave wave = LoadWave(playFileName);     // Load audio (WAV, OGG, FLAC, MP3)
        PlayWaveCLI(wave);
        UnloadWave(wave);
    }

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif      // PLATFORM_DESKTOP

//--------------------------------------------------------------------------------------------
// Load/Save/Export functions
//--------------------------------------------------------------------------------------------

// Reset wave parameters
static void ResetWaveParams(WaveParams *params)
{
    // NOTE: Random seed is set to a random value
    params->randSeed = GetRandomValue(0x1, 0xFFFE);
    srand(params->randSeed);

    // Wave type
    params->waveTypeValue = 0;

    // Wave envelope params
    params->attackTimeValue = 0.0f;
    params->sustainTimeValue = 0.3f;
    params->sustainPunchValue = 0.0f;
    params->decayTimeValue = 0.4f;

    // Frequency params
    params->startFrequencyValue = 0.3f;
    params->minFrequencyValue = 0.0f;
    params->slideValue = 0.0f;
    params->deltaSlideValue = 0.0f;
    params->vibratoDepthValue = 0.0f;
    params->vibratoSpeedValue = 0.0f;
    //params->vibratoPhaseDelay = 0.0f;

    // Tone change params
    params->changeAmountValue = 0.0f;
    params->changeSpeedValue = 0.0f;

    // Square wave params
    params->squareDutyValue = 0.0f;
    params->dutySweepValue = 0.0f;

    // Repeat params
    params->repeatSpeedValue = 0.0f;

    // Phaser params
    params->phaserOffsetValue = 0.0f;
    params->phaserSweepValue = 0.0f;

    // Filter params
    params->lpfCutoffValue = 1.0f;
    params->lpfCutoffSweepValue = 0.0f;
    params->lpfResonanceValue = 0.0f;
    params->hpfCutoffValue = 0.0f;
    params->hpfCutoffSweepValue = 0.0f;
}

// Generates new wave from wave parameters
// NOTE: By default wave is generated as 44100Hz, 32bit float, mono
static Wave GenerateWave(WaveParams params)
{
    #define MAX_WAVE_LENGTH_SECONDS  10     // Max length for wave: 10 seconds
    #define WAVE_SAMPLE_RATE      44100     // Default sample rate

    #define rnd(n) (rand()%(n + 1))
    #define GetRandomFloat(range) ((float)rnd(10000)/10000*range)

    if (params.randSeed != 0) srand(params.randSeed);   // Initialize seed if required

    // Configuration parameters for generation
    // NOTE: Those parameters are calculated from selected values
    int phase = 0;
    double fperiod = 0.0;
    double fmaxperiod = 0.0;
    double fslide = 0.0;
    double fdslide = 0.0;
    int period = 0;
    float squareDuty = 0.0f;
    float squareSlide = 0.0f;
    int envelopeStage = 0;
    int envelopeTime = 0;
    int envelopeLength[3] = { 0 };
    float envelopeVolume = 0.0f;
    float fphase = 0.0f;
    float fdphase = 0.0f;
    int iphase = 0;
    float phaserBuffer[1024] = { 0 };
    int ipp = 0;
    float noiseBuffer[32] = { 0 };       // Required for noise wave, depends on random seed!
    float fltp = 0.0f;
    float fltdp = 0.0f;
    float fltw = 0.0f;
    float fltwd = 0.0f;
    float fltdmp = 0.0f;
    float fltphp = 0.0f;
    float flthp = 0.0f;
    float flthpd = 0.0f;
    float vibratoPhase = 0.0f;
    float vibratoSpeed = 0.0f;
    float vibratoAmplitude = 0.0f;
    int repeatTime = 0;
    int repeatLimit = 0;
    int arpeggioTime = 0;
    int arpeggioLimit = 0;
    double arpeggioModulation = 0.0;

    // HACK: Security check to avoid crash (why?)
    if (params.minFrequencyValue > params.startFrequencyValue) params.minFrequencyValue = params.startFrequencyValue;
    if (params.slideValue < params.deltaSlideValue) params.slideValue = params.deltaSlideValue;

    // Reset sample parameters
    //----------------------------------------------------------------------------------------
    fperiod = 100.0/(params.startFrequencyValue*params.startFrequencyValue + 0.001);
    period = (int)fperiod;
    fmaxperiod = 100.0/(params.minFrequencyValue*params.minFrequencyValue + 0.001);
    fslide = 1.0 - pow((double)params.slideValue, 3.0)*0.01;
    fdslide = -pow((double)params.deltaSlideValue, 3.0)*0.000001;
    squareDuty = 0.5f - params.squareDutyValue*0.5f;
    squareSlide = -params.dutySweepValue*0.00005f;

    if (params.changeAmountValue >= 0.0f) arpeggioModulation = 1.0 - pow((double)params.changeAmountValue, 2.0)*0.9;
    else arpeggioModulation = 1.0 + pow((double)params.changeAmountValue, 2.0)*10.0;

    arpeggioLimit = (int)(powf(1.0f - params.changeSpeedValue, 2.0f)*20000 + 32);

    if (params.changeSpeedValue == 1.0f) arpeggioLimit = 0;     // WATCH OUT: float comparison

    // Reset filter parameters
    fltw = powf(params.lpfCutoffValue, 3.0f)*0.1f;
    fltwd = 1.0f + params.lpfCutoffSweepValue*0.0001f;
    fltdmp = 5.0f/(1.0f + powf(params.lpfResonanceValue, 2.0f)*20.0f)*(0.01f + fltw);
    if (fltdmp > 0.8f) fltdmp = 0.8f;
    flthp = powf(params.hpfCutoffValue, 2.0f)*0.1f;
    flthpd = 1.0f + params.hpfCutoffSweepValue*0.0003f;

    // Reset vibrato
    vibratoSpeed = powf(params.vibratoSpeedValue, 2.0f)*0.01f;
    vibratoAmplitude = params.vibratoDepthValue*0.5f;

    // Reset envelope
    envelopeLength[0] = (int)(params.attackTimeValue*params.attackTimeValue*100000.0f);
    envelopeLength[1] = (int)(params.sustainTimeValue*params.sustainTimeValue*100000.0f);
    envelopeLength[2] = (int)(params.decayTimeValue*params.decayTimeValue*100000.0f);

    fphase = powf(params.phaserOffsetValue, 2.0f)*1020.0f;
    if (params.phaserOffsetValue < 0.0f) fphase = -fphase;

    fdphase = powf(params.phaserSweepValue, 2.0f)*1.0f;
    if (params.phaserSweepValue < 0.0f) fdphase = -fdphase;

    iphase = abs((int)fphase);

    for (int i = 0; i < 32; i++) noiseBuffer[i] = GetRandomFloat(2.0f) - 1.0f;      // WATCH OUT: GetRandomFloat()

    repeatLimit = (int)(powf(1.0f - params.repeatSpeedValue, 2.0f)*20000 + 32);

    if (params.repeatSpeedValue == 0.0f) repeatLimit = 0;
    //----------------------------------------------------------------------------------------

    // NOTE: We reserve enough space for up to 10 seconds of wave audio at given sample rate
    // By default we use float size samples, they are converted to desired sample size at the end
    float *buffer = (float *)RL_CALLOC(MAX_WAVE_LENGTH_SECONDS*WAVE_SAMPLE_RATE, sizeof(float));
    bool generatingSample = true;
    int sampleCount = 0;

    for (int i = 0; i < MAX_WAVE_LENGTH_SECONDS*WAVE_SAMPLE_RATE; i++)
    {
        if (!generatingSample)
        {
            sampleCount = i;
            break;
        }

        // Generate sample using selected parameters
        //------------------------------------------------------------------------------------
        repeatTime++;

        if ((repeatLimit != 0) && (repeatTime >= repeatLimit))
        {
            // Reset sample parameters (only some of them)
            repeatTime = 0;

            fperiod = 100.0/(params.startFrequencyValue*params.startFrequencyValue + 0.001);
            period = (int)fperiod;
            fmaxperiod = 100.0/(params.minFrequencyValue*params.minFrequencyValue + 0.001);
            fslide = 1.0 - pow((double)params.slideValue, 3.0)*0.01;
            fdslide = -pow((double)params.deltaSlideValue, 3.0)*0.000001;
            squareDuty = 0.5f - params.squareDutyValue*0.5f;
            squareSlide = -params.dutySweepValue*0.00005f;

            if (params.changeAmountValue >= 0.0f) arpeggioModulation = 1.0 - pow((double)params.changeAmountValue, 2.0)*0.9;
            else arpeggioModulation = 1.0 + pow((double)params.changeAmountValue, 2.0)*10.0;

            arpeggioTime = 0;
            arpeggioLimit = (int)(powf(1.0f - params.changeSpeedValue, 2.0f)*20000 + 32);

            if (params.changeSpeedValue == 1.0f) arpeggioLimit = 0;     // WATCH OUT: float comparison
        }

        // Frequency envelopes/arpeggios
        arpeggioTime++;

        if ((arpeggioLimit != 0) && (arpeggioTime >= arpeggioLimit))
        {
            arpeggioLimit = 0;
            fperiod *= arpeggioModulation;
        }

        fslide += fdslide;
        fperiod *= fslide;

        if (fperiod > fmaxperiod)
        {
            fperiod = fmaxperiod;

            if (params.minFrequencyValue > 0.0f) generatingSample = false;
        }

        float rfperiod = (float)fperiod;

        if (vibratoAmplitude > 0.0f)
        {
            vibratoPhase += vibratoSpeed;
            rfperiod = (float)(fperiod*(1.0 + sinf(vibratoPhase)*vibratoAmplitude));
        }

        period = (int)rfperiod;

        if (period < 8) period=8;

        squareDuty += squareSlide;

        if (squareDuty < 0.0f) squareDuty = 0.0f;
        if (squareDuty > 0.5f) squareDuty = 0.5f;

        // Volume envelope
        envelopeTime++;

        if (envelopeTime > envelopeLength[envelopeStage])
        {
            envelopeTime = 0;
            envelopeStage++;

            if (envelopeStage == 3) generatingSample = false;
        }

        if (envelopeStage == 0) envelopeVolume = (float)envelopeTime/envelopeLength[0];
        if (envelopeStage == 1) envelopeVolume = 1.0f + powf(1.0f - (float)envelopeTime/envelopeLength[1], 1.0f)*2.0f*params.sustainPunchValue;
        if (envelopeStage == 2) envelopeVolume = 1.0f - (float)envelopeTime/envelopeLength[2];

        // Phaser step
        fphase += fdphase;
        iphase = abs((int)fphase);

        if (iphase > 1023) iphase = 1023;

        if (flthpd != 0.0f)     // WATCH OUT!
        {
            flthp *= flthpd;
            if (flthp < 0.00001f) flthp = 0.00001f;
            if (flthp > 0.1f) flthp = 0.1f;
        }

        float ssample = 0.0f;

        #define MAX_SUPERSAMPLING   8

        // Supersampling x8
        for (int si = 0; si < MAX_SUPERSAMPLING; si++)
        {
            float sample = 0.0f;
            phase++;

            if (phase >= period)
            {
                //phase = 0;
                phase %= period;

                if (params.waveTypeValue == 3)
                {
                    for (int i = 0;i < 32; i++) noiseBuffer[i] = GetRandomFloat(2.0f) - 1.0f;   // WATCH OUT: GetRandomFloat()
                }
            }

            // base waveform
            float fp = (float)phase/period;

            switch (params.waveTypeValue)
            {
                case 0: // Square wave
                {
                    if (fp < squareDuty) sample = 0.5f;
                    else sample = -0.5f;

                } break;
                case 1: sample = 1.0f - fp*2; break;    // Sawtooth wave
                case 2: sample = sinf(fp*2*PI); break;  // Sine wave
                case 3: sample = noiseBuffer[phase*32/period]; break; // Noise wave
                default: break;
            }

            // LP filter
            float pp = fltp;
            fltw *= fltwd;

            if (fltw < 0.0f) fltw = 0.0f;
            if (fltw > 0.1f) fltw = 0.1f;

            if (params.lpfCutoffValue != 1.0f)  // WATCH OUT!
            {
                fltdp += (sample-fltp)*fltw;
                fltdp -= fltdp*fltdmp;
            }
            else
            {
                fltp = sample;
                fltdp = 0.0f;
            }

            fltp += fltdp;

            // HP filter
            fltphp += fltp - pp;
            fltphp -= fltphp*flthp;
            sample = fltphp;

            // Phaser
            phaserBuffer[ipp & 1023] = sample;
            sample += phaserBuffer[(ipp - iphase + 1024) & 1023];
            ipp = (ipp + 1) & 1023;

            // Final accumulation and envelope application
            ssample += sample*envelopeVolume;
        }

        #define SAMPLE_SCALE_COEFICIENT 0.2f    // NOTE: Used to scale sample value to [-1..1]

        ssample = (ssample/MAX_SUPERSAMPLING)*SAMPLE_SCALE_COEFICIENT;
        //------------------------------------------------------------------------------------

        // Accumulate samples in the buffer
        if (ssample > 1.0f) ssample = 1.0f;
        if (ssample < -1.0f) ssample = -1.0f;

        buffer[i] = ssample;
    }

    Wave genWave = { 0 };
    genWave.frameCount = sampleCount/1;    // Number of samples / channels
    genWave.sampleRate = WAVE_SAMPLE_RATE; // By default 44100 Hz
    genWave.sampleSize = 32;               // By default 32 bit float samples
    genWave.channels = 1;                  // By default 1 channel (mono)

    // NOTE: Wave can be converted to desired format after generation

    genWave.data = (float *)RL_CALLOC(genWave.frameCount*genWave.channels, sizeof(float));
    memcpy(genWave.data, buffer, genWave.frameCount*genWave.channels*sizeof(float));

    RL_FREE(buffer);

    return genWave;
}

// Load .rfx (rFXGen) or .sfs (sfxr) sound parameters file
static WaveParams LoadWaveParams(const char *fileName)
{
    WaveParams params = { 0 };

    if (IsFileExtension(fileName, ".rfx"))
    {
        FILE *rfxFile = fopen(fileName, "rb");

        if (rfxFile != NULL)
        {
            // Read .rfx file header
            unsigned char signature[5] = { 0 };
            fread(signature, 4, sizeof(unsigned char), rfxFile);

            // Check for valid .rfx file (FormatCC)
            if ((signature[0] == 'r') &&
                (signature[1] == 'F') &&
                (signature[2] == 'X') &&
                (signature[3] == ' '))
            {
                unsigned short version = 0;
                unsigned short length = 0;
                fread(&version, 1, sizeof(unsigned short), rfxFile);
                fread(&length, 1, sizeof(unsigned short), rfxFile);

                if (version != 200) LOG("[%s] rFX file version not supported (%i)\n", fileName, version);
                else
                {
                    if (length != sizeof(WaveParams)) LOG("[%s] Wrong rFX wave parameters size\n", fileName);
                    else fread(&params, 1, sizeof(WaveParams), rfxFile);   // Load wave generation parameters
                }
            }
            else LOG("[%s] rFX file does not seem to be valid\n", fileName);

            fclose(rfxFile);
        }
    }
    /*
    else if (IsFileExtension(fileName, ".sfs"))
    {
        // NOTE: It seem .sfs loading has some issues,
        // I can't see the point to keep supporting this format

        FILE *sfsFile = fopen(fileName, "rb");

        if (sfsFile == NULL) return params;

        // Load .sfs sound parameters
        int version = 0;
        fread(&version, 1, sizeof(int), sfsFile);

        if ((version == 100) || (version == 101) || (version == 102))
        {
            fread(&params.waveTypeValue, 1, sizeof(int), sfsFile);

            volumeValue = 0.5f;

            if (version == 102) fread(&volumeValue, 1, sizeof(float), sfsFile);

            fread(&params.startFrequencyValue, 1, sizeof(float), sfsFile);
            fread(&params.minFrequencyValue, 1, sizeof(float), sfsFile);
            fread(&params.slideValue, 1, sizeof(float), sfsFile);

            if (version >= 101) fread(&params.deltaSlideValue, 1, sizeof(float), sfsFile);

            fread(&params.squareDutyValue, 1, sizeof(float), sfsFile);
            fread(&params.dutySweepValue, 1, sizeof(float), sfsFile);

            fread(&params.vibratoDepthValue, 1, sizeof(float), sfsFile);
            fread(&params.vibratoSpeedValue, 1, sizeof(float), sfsFile);

            float vibratoPhaseDelay = 0.0f;
            fread(&vibratoPhaseDelay, 1, sizeof(float), sfsFile); // Not used

            fread(&params.attackTimeValue, 1, sizeof(float), sfsFile);
            fread(&params.sustainTimeValue, 1, sizeof(float), sfsFile);
            fread(&params.decayTimeValue, 1, sizeof(float), sfsFile);
            fread(&params.sustainPunchValue, 1, sizeof(float), sfsFile);

            bool filterOn = false;
            fread(&filterOn, 1, sizeof(bool), sfsFile); // Not used

            fread(&params.lpfResonanceValue, 1, sizeof(float), sfsFile);
            fread(&params.lpfCutoffValue, 1, sizeof(float), sfsFile);
            fread(&params.lpfCutoffSweepValue, 1, sizeof(float), sfsFile);
            fread(&params.hpfCutoffValue, 1, sizeof(float), sfsFile);
            fread(&params.hpfCutoffSweepValue, 1, sizeof(float), sfsFile);

            fread(&params.phaserOffsetValue, 1, sizeof(float), sfsFile);
            fread(&params.phaserSweepValue, 1, sizeof(float), sfsFile);
            fread(&params.repeatSpeedValue, 1, sizeof(float), sfsFile);

            if (version >= 101)
            {
                fread(&params.changeSpeedValue, 1, sizeof(float), sfsFile);
                fread(&params.changeAmountValue, 1, sizeof(float), sfsFile);
            }
        }
        else LOG("[%s] SFS file version not supported\n", fileName);

        fclose(sfsFile);
    }
    */

    return params;
}

// Save .rfx sound parameters file
static void SaveWaveParams(WaveParams params, const char *fileName)
{
    if (IsFileExtension(fileName, ".rfx"))
    {
        // Fx Sound File Structure (.rfx)
        // ------------------------------------------------------
        // Offset | Size  | Type       | Description
        // ------------------------------------------------------
        // 0      | 4     | char       | Signature: "rFX "
        // 4      | 2     | short      | Version: 200
        // 6      | 2     | short      | Data length: 96 bytes
        // 8      | 96    | WaveParams | Wave parameters
        // ------------------------------------------------------

        FILE *rfxFile = fopen(fileName, "wb");

        if (rfxFile != NULL)
        {
            unsigned char signature[5] = "rFX ";
            unsigned short version = 200;
            unsigned short length = sizeof(WaveParams);

            // Write .rfx file header
            fwrite(signature, 4, sizeof(unsigned char), rfxFile);
            fwrite(&version, 1, sizeof(unsigned short), rfxFile);
            fwrite(&length, 1, sizeof(unsigned short), rfxFile);

            // Write wave generation parameters
            fwrite(&params, 1, sizeof(WaveParams), rfxFile);

            fclose(rfxFile);
        }
    }
}

//--------------------------------------------------------------------------------------------
// Sound generation functions
//--------------------------------------------------------------------------------------------

// Generate sound: Pickup/Coin
static WaveParams GenPickupCoin(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.startFrequencyValue = 0.4f + frnd(0.5f);
    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = frnd(0.1f);
    params.decayTimeValue = 0.1f + frnd(0.4f);
    params.sustainPunchValue = 0.3f + frnd(0.3f);

    if (GetRandomValue(0, 1))
    {
        params.changeSpeedValue = 0.5f + frnd(0.2f);
        params.changeAmountValue = 0.2f + frnd(0.4f);
    }

    return params;
}

// Generate sound: Laser shoot
static WaveParams GenLaserShoot(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.waveTypeValue = GetRandomValue(0, 2);

    if ((params.waveTypeValue == 2) && GetRandomValue(0, 1)) params.waveTypeValue = GetRandomValue(0, 1);

    params.startFrequencyValue = 0.5f + frnd(0.5f);
    params.minFrequencyValue = params.startFrequencyValue - 0.2f - frnd(0.6f);

    if (params.minFrequencyValue < 0.2f) params.minFrequencyValue = 0.2f;

    params.slideValue = -0.15f - frnd(0.2f);

    if (GetRandomValue(0, 2) == 0)
    {
        params.startFrequencyValue = 0.3f + frnd(0.6f);
        params.minFrequencyValue = frnd(0.1f);
        params.slideValue = -0.35f - frnd(0.3f);
    }

    if (GetRandomValue(0, 1))
    {
        params.squareDutyValue = frnd(0.5f);
        params.dutySweepValue = frnd(0.2f);
    }
    else
    {
        params.squareDutyValue = 0.4f + frnd(0.5f);
        params.dutySweepValue = -frnd(0.7f);
    }

    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = 0.1f + frnd(0.2f);
    params.decayTimeValue = frnd(0.4f);

    if (GetRandomValue(0, 1)) params.sustainPunchValue = frnd(0.3f);

    if (GetRandomValue(0, 2) == 0)
    {
        params.phaserOffsetValue = frnd(0.2f);
        params.phaserSweepValue = -frnd(0.2f);
    }

    if (GetRandomValue(0, 1)) params.hpfCutoffValue = frnd(0.3f);

    return params;
}

// Generate sound: Explosion
static WaveParams GenExplosion(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.waveTypeValue = 3;

    if (GetRandomValue(0, 1))
    {
        params.startFrequencyValue = 0.1f + frnd(0.4f);
        params.slideValue = -0.1f + frnd(0.4f);
    }
    else
    {
        params.startFrequencyValue = 0.2f + frnd(0.7f);
        params.slideValue = -0.2f - frnd(0.2f);
    }

    params.startFrequencyValue *= params.startFrequencyValue;

    if (GetRandomValue(0, 4) == 0) params.slideValue = 0.0f;
    if (GetRandomValue(0, 2) == 0) params.repeatSpeedValue = 0.3f + frnd(0.5f);

    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = 0.1f + frnd(0.3f);
    params.decayTimeValue = frnd(0.5f);

    if (GetRandomValue(0, 1) == 0)
    {
        params.phaserOffsetValue = -0.3f + frnd(0.9f);
        params.phaserSweepValue = -frnd(0.3f);
    }

    params.sustainPunchValue = 0.2f + frnd(0.6f);

    if (GetRandomValue(0, 1))
    {
        params.vibratoDepthValue = frnd(0.7f);
        params.vibratoSpeedValue = frnd(0.6f);
    }

    if (GetRandomValue(0, 2) == 0)
    {
        params.changeSpeedValue = 0.6f + frnd(0.3f);
        params.changeAmountValue = 0.8f - frnd(1.6f);
    }

    return params;
}

// Generate sound: Powerup
static WaveParams GenPowerup(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    if (GetRandomValue(0, 1)) params.waveTypeValue = 1;
    else params.squareDutyValue = frnd(0.6f);

    if (GetRandomValue(0, 1))
    {
        params.startFrequencyValue = 0.2f + frnd(0.3f);
        params.slideValue = 0.1f + frnd(0.4f);
        params.repeatSpeedValue = 0.4f + frnd(0.4f);
    }
    else
    {
        params.startFrequencyValue = 0.2f + frnd(0.3f);
        params.slideValue = 0.05f + frnd(0.2f);

        if (GetRandomValue(0, 1))
        {
            params.vibratoDepthValue = frnd(0.7f);
            params.vibratoSpeedValue = frnd(0.6f);
        }
    }

    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = frnd(0.4f);
    params.decayTimeValue = 0.1f + frnd(0.4f);

    return params;
}

// Generate sound: Hit/Hurt
static WaveParams GenHitHurt(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.waveTypeValue = GetRandomValue(0, 2);
    if (params.waveTypeValue == 2) params.waveTypeValue = 3;
    if (params.waveTypeValue == 0) params.squareDutyValue = frnd(0.6f);

    params.startFrequencyValue = 0.2f + frnd(0.6f);
    params.slideValue = -0.3f - frnd(0.4f);
    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = frnd(0.1f);
    params.decayTimeValue = 0.1f + frnd(0.2f);

    if (GetRandomValue(0, 1)) params.hpfCutoffValue = frnd(0.3f);

    return params;
}

// Generate sound: Jump
static WaveParams GenJump(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.waveTypeValue = 0;
    params.squareDutyValue = frnd(0.6f);
    params.startFrequencyValue = 0.3f + frnd(0.3f);
    params.slideValue = 0.1f + frnd(0.2f);
    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = 0.1f + frnd(0.3f);
    params.decayTimeValue = 0.1f + frnd(0.2f);

    if (GetRandomValue(0, 1)) params.hpfCutoffValue = frnd(0.3f);
    if (GetRandomValue(0, 1)) params.lpfCutoffValue = 1.0f - frnd(0.6f);

    return params;
}

// Generate sound: Blip/Select
static WaveParams GenBlipSelect(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.waveTypeValue = GetRandomValue(0, 1);
    if (params.waveTypeValue == 0) params.squareDutyValue = frnd(0.6f);
    params.startFrequencyValue = 0.2f + frnd(0.4f);
    params.attackTimeValue = 0.0f;
    params.sustainTimeValue = 0.1f + frnd(0.1f);
    params.decayTimeValue = frnd(0.2f);
    params.hpfCutoffValue = 0.1f;

    return params;
}

// Generate random sound
static WaveParams GenRandomize(void)
{
    WaveParams params = { 0 };
    ResetWaveParams(&params);

    params.randSeed = GetRandomValue(0, 0xFFFE);

    params.startFrequencyValue = powf(frnd(2.0f) - 1.0f, 2.0f);

    if (GetRandomValue(0, 1)) params.startFrequencyValue = powf(frnd(2.0f) - 1.0f, 3.0f)+0.5f;

    params.minFrequencyValue = 0.0f;
    params.slideValue = powf(frnd(2.0f) - 1.0f, 5.0f);

    if ((params.startFrequencyValue > 0.7f) && (params.slideValue > 0.2f)) params.slideValue = -params.slideValue;
    if ((params.startFrequencyValue < 0.2f) && (params.slideValue < -0.05f)) params.slideValue = -params.slideValue;

    params.deltaSlideValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.squareDutyValue = frnd(2.0f) - 1.0f;
    params.dutySweepValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.vibratoDepthValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.vibratoSpeedValue = frnd(2.0f) - 1.0f;
    //params.vibratoPhaseDelay = frnd(2.0f) - 1.0f;
    params.attackTimeValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.sustainTimeValue = powf(frnd(2.0f) - 1.0f, 2.0f);
    params.decayTimeValue = frnd(2.0f)-1.0f;
    params.sustainPunchValue = powf(frnd(0.8f), 2.0f);

    if (params.attackTimeValue + params.sustainTimeValue + params.decayTimeValue < 0.2f)
    {
        params.sustainTimeValue += 0.2f + frnd(0.3f);
        params.decayTimeValue += 0.2f + frnd(0.3f);
    }

    params.lpfResonanceValue = frnd(2.0f) - 1.0f;
    params.lpfCutoffValue = 1.0f - powf(frnd(1.0f), 3.0f);
    params.lpfCutoffSweepValue = powf(frnd(2.0f) - 1.0f, 3.0f);

    if (params.lpfCutoffValue < 0.1f && params.lpfCutoffSweepValue < -0.05f) params.lpfCutoffSweepValue = -params.lpfCutoffSweepValue;

    params.hpfCutoffValue = powf(frnd(1.0f), 5.0f);
    params.hpfCutoffSweepValue = powf(frnd(2.0f) - 1.0f, 5.0f);
    params.phaserOffsetValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.phaserSweepValue = powf(frnd(2.0f) - 1.0f, 3.0f);
    params.repeatSpeedValue = frnd(2.0f) - 1.0f;
    params.changeSpeedValue = frnd(2.0f) - 1.0f;
    params.changeAmountValue = frnd(2.0f) - 1.0f;

    return params;
}

// Mutate current sound
static void WaveMutate(WaveParams *params)
{
    srand(time(NULL));      // Refresh seed to avoid converging behaviour
    
    if (GetRandomValue(0, 1)) params->startFrequencyValue += frnd(0.1f) - 0.05f;        
    //if (GetRandomValue(0, 1)) params.minFrequencyValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->slideValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->deltaSlideValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->squareDutyValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->dutySweepValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->vibratoDepthValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->vibratoSpeedValue += frnd(0.1f) - 0.05f;
    //if (GetRandomValue(0, 1)) params.vibratoPhaseDelay += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->attackTimeValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->sustainTimeValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->decayTimeValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->sustainPunchValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->lpfResonanceValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->lpfCutoffValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->lpfCutoffSweepValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->hpfCutoffValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->hpfCutoffSweepValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->phaserOffsetValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->phaserSweepValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->repeatSpeedValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->changeSpeedValue += frnd(0.1f) - 0.05f;
    if (GetRandomValue(0, 1)) params->changeAmountValue += frnd(0.1f) - 0.05f;
}

//--------------------------------------------------------------------------------------------
// Auxiliar functions
//--------------------------------------------------------------------------------------------
// Draw wave data
// NOTE: For proper visualization, MSAA x4 is recommended but it could be costly for the GPU
// Alternative: Rendered to a bigger texture and scale down with bilinear/trilinear texture filtering
static void DrawWave(Wave *wave, Rectangle bounds, Color color)
{
    float sample = 0.0f;
    float sampleNext = 0.0f;
    float currentSample = 0.0f;
    float sampleIncrement = (float)wave->frameCount*wave->channels/(float)(bounds.width*2);
    float sampleScale = (float)bounds.height;

    for (int i = 1; i < bounds.width*2 - 1; i++)
    {
        sample = ((float *)wave->data)[(int)currentSample]*sampleScale;
        sampleNext = ((float *)wave->data)[(int)(currentSample + sampleIncrement)]*sampleScale;

        if (sample > bounds.height/2) sample = bounds.height/2;
        else if (sample < -bounds.height/2) sample = -bounds.height/2;

        if (sampleNext > bounds.height/2) sampleNext = bounds.height/2;
        else if (sampleNext < -bounds.height/2) sampleNext = -bounds.height/2;

        DrawLineV((Vector2){ (float)bounds.x + (float)i/2.0f, (float)(bounds.y + bounds.height/2) + sample },
                  (Vector2){ (float)bounds.x + (float)i/2.0f, (float)(bounds.y  + bounds.height/2) + sampleNext }, color);

        currentSample += sampleIncrement;
    }
}

// Draw help window with the provided lines
static int GuiHelpWindow(Rectangle bounds, const char *title, const char **helpLines, int helpLinesCount)
{
    int nextLineY = 0;

    // Calculate window height if not externally provided a desired height
    if (bounds.height == 0) bounds.height = (float)(helpLinesCount*24 + 24);

    int helpWindowActive = !GuiWindowBox(bounds, title);
    nextLineY += (24 + 2);

    for (int i = 0; i < helpLinesCount; i++)
    {
        if (helpLines[i] == NULL) GuiLine((Rectangle) { bounds.x, bounds.y + nextLineY, 330, 12 }, helpLines[i]);
        else if (helpLines[i][0] == '-') GuiLine((Rectangle) { bounds.x, bounds.y + nextLineY, 330, 24 }, helpLines[i] + 1);
        else GuiLabel((Rectangle) { bounds.x + 12, bounds.y + nextLineY, 0, 24 }, helpLines[i]);

        if (helpLines[i] == NULL) nextLineY += 12;
        else nextLineY += 24;
    }

    return helpWindowActive;
}

#if defined(PLATFORM_DESKTOP)
// Simple time wait in milliseconds
static void WaitTimePlayer(int ms)
{
    if (ms > 0)
    {
        int currentTime = clock()*1000/CLOCKS_PER_SEC;  // Current time in milliseconds
        int totalTime = currentTime + ms;               // Total required time in ms to return from this timeout

        int percent = 0;
        int prevPercent = percent;

        // Wait until current ms time matches total ms time
        while (currentTime <= totalTime)
        {
            // Check for key pressed to stop playing
            if (kbhit())
            {
                int key = getch();
                if ((key == 13) || (key == 27)) break;    // KEY_ENTER || KEY_ESCAPE
            }

            currentTime = clock()*1000/CLOCKS_PER_SEC;

            // Print console time bar
            percent = (int)(((float)currentTime/totalTime)*100.0f);

            if (percent != prevPercent)
            {
                LOG("\r[");
                for (int j = 0; j < 50; j++)
                {
                    if (j < percent/2) LOG("=");
                    else LOG(" ");
                }
                LOG("] [%02i%%]", percent);

                prevPercent = percent;
            }
        }

        LOG("\n\n");
    }
}

// Play provided wave through CLI
static void PlayWaveCLI(Wave wave)
{
    float waveTimeMs = (float)wave.frameCount*1000.0f/wave.sampleRate;

    InitAudioDevice();                  // Init audio device
    Sound fx = LoadSoundFromWave(wave); // Load audio wave

    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// %s v%s - CLI audio player                                         //\n", toolName, toolVersion);
    printf("//                                                                              //\n");
    printf("// more info and bugs-report: github.com/raysan5/rfxgen                         //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2020-2022 raylib technologies (@raylibtech)                    //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("Playing sound [%.2f sec.]. Press ENTER to finish.\n", waveTimeMs/1000.0f);

    PlaySound(fx);                      // Play sound
    WaitTimePlayer(waveTimeMs);         // Wait while audio is playing
    UnloadSound(fx);                    // Unload sound data
    CloseAudioDevice();                 // Close audio device
}

#if !defined(_WIN32)
// Check if a key has been pressed
static int kbhit(void)
{
    struct termios oldt = { 0 };
    struct termios newt = { 0 };
    int ch = 0;
    int oldf = 0;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif
#endif      // PLATFORM_DESKTOP
