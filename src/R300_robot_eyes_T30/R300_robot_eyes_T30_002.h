
#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <string.h> // for strcmp, strdup

// Relevant CONFIGURATION Defines from config.h
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW  // Change this to your hardware type
#define MAX_DEVICES 2
#define CS_PIN 5

// Misc defines from persona.h
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))  ///< number of elements in an array
#define EYE_COL_SIZE 8                            ///< number of columns in one eye

// Module offsets from first module specified
#define LEFT_MODULE_OFFSET 1   ///< offset from the base LED module for the left eye
#define RIGHT_MODULE_OFFSET 0  ///< offset from the base LED module for the right eye

// Array references for eyeData array below
#define LEFT_EYE_INDEX 1   ///< array reference in the eye data for the left eye
#define RIGHT_EYE_INDEX 0  ///< array reference in the eye data for the right eye

// Basic unit of time a frame is displayed
#define FRAME_TIME 100  ///< minimum animation time

// State enum from persona.h
typedef enum {
    AWAKE,
    SLEEPING,
    ANGRY, // Although defined, it's also an emotion type
} State;

/**
 * Robot Eyes Class.
 * This class manages the displayed of animated eyes using LED matrices using the functions
 * provided by the MD_MAX72xx library.
 */
class Persona {
   public:
    /**
     * Emotions enumerated type.
     *
     * This enumerated type defines the emotion animations
     * available in the class for the eyes display
     */
    typedef enum {
        E_NONE,          ///< placeholder for no emotions, not user selectable
        E_NEUTRAL,       ///< eyes in neutral position (no animation)
        E_BLINK,         ///< both eyes blink
        E_WINK,          ///< one eye blink
        E_LOOK_L,        ///< both eyes look left
        E_LOOK_R,        ///< both eyes look right
        E_LOOK_U,        ///< both eyes look up
        E_LOOK_D,        ///< both eyes look down
        E_ANGRY,         ///< eyes look angry (symmetrical)
        E_SAD,           ///< eyes look sad (symmetrical)
        E_EVIL,          ///< eyes look evil (symmetrical)
        E_EVIL2,         ///< eyes look evil (asymmetrical)
        E_SQUINT,        ///< both eye squint
        E_DEAD,          ///< eyes indicate dead (different)
        E_SCAN_UD,       ///< both eyes scanning Up/Down
        E_SCAN_LR,       ///< both eyes scanning Left/Right
        E_SQUINT_BLINK,  ///< both eyes squint and blink
        E_IDLE,          ///< eyes in idle position (no animation)
        E_HEART,         ///< eyes display a heart,
        E_SLEEP,         ///< eyes in sleep position
    } emotion_t;

    /**
     * Class Constructor.
     *
     * Instantiate a new instance of the class.
     */
    Persona(void);

    /**
     * Class Destructor.
     *
     * Released any allocated memory and does the necessary to clean
     * up once the object is no longer required.
     */
    ~Persona(void) {};

    /**
     * Initialize the object.
     *
     * Initialize the object data. This needs to be called during setup() to initialize new
     * data for the class that cannot be done during the object creation.
     *
     * Outside of the class, the MD_MAX72xx library should be initialized and the pointer
     * to the MD_MAX72xx object passed to the parameter. Also, as the eyes could be in the
     * middle of a string of LED modules, the first 'eye' module can be specified.
     *
     * /param M            pointer to the MD_MAX72xx library object. (Removed - MD_MAX72xx is global)
     * /param moduleStart  the first 'eye' LED module. Defaults to 0 if not specified.
     */
    void begin(uint8_t moduleStart = 0);

    /**
     * Set the animation type and parameters.
     *
     * Set the next animations to the specified. Additionally, set whether the animation should
     * auto reverse the action (eg, blink down then back up again) and whether the animation
     * should be run in reverse.
     *
     * Animations are generally symmetric, so only half the animation needs to be specified.
     * If an animated expression needs to be held, the animation should be run without auto
     * reverse, which holds the animation at the end point, and then later run the animation
     * in reverse from the last position to return to the idle state.
     *
     * \param e  the type of emotion to be displayed, one of the emotion_T enumerated values.
     * \param r  if true, run auto reverse.
     * \param b  if true, start the animation from the end of the sequence.
     * \param force  if true, force the animation to start from the beginning.
     */
    inline void setAnimation(emotion_t e, bool r, bool b = false, bool force = false) {
        _nextEmotion = e;
        _autoReverse = r;
        _animReverse = b;
        if (force && _state == AWAKE && e != _currentEmotion) {
            _animState = S_IDLE;
        }
    }

    /**
     * Set the blink time.
     *
     * When no animation is running and AutoBlink is set, the eyes will occasionally blink.
     * Set the minimum time period between blinks. A blink will occur a random time after this.
     *
     * \param t  the minimum time between blinking actions in milliseconds.
     */
    inline void setBlinkTime(uint16_t t) { _timeBlinkMinimum = t; };

    /**
     * Set or reset auto blink mode.
     *
     * When no animation is running and AutoBlink is set, the eyes will occasionally blink.
     *
     * \param b  set auto blink if true, reset auto blink if false.
     */
    inline void setAutoBlink(bool b) { _autoBlink = b; };

    /**
    * Display a text message.
    *
    * At the end of the current animation, the text will be scrolled across the 'eyes'
    * and then the eyes are returned to the neutral expression
    *
    * \param p  a pointer to a char array containing a nul terminated string.
                The string must remain in scope while the message is being displayed.
    */
    inline bool setText(const char* pText) {
        if (_pText != nullptr)
            return (false);
        else
            _pText = pText;
        return (true);
    };

    /**
     * Animate the display.
     *
     * This method needs to be invoked as often as possible to ensure smooth animation.
     *
     * The calling program should monitor the return value for 'true' in order to know when
     * the animation has concluded. A 'true' return value means that the animation is complete.
     *
     * \return bool  true if the animation has completed, false otherwise.
     */
    bool runAnimation(void);

    void setState(State state);

    void processCommand(const char* command);

   protected:
    // Animations FSM state
    typedef enum {
        S_IDLE,
        S_RESTART,
        S_ANIMATE,
        S_PAUSE,
        S_TEXT,
    } animState_t;

    // Define an animation frame
    struct animFrame_t {
        uint8_t eyeData[2];  // [LEFT_MODULE_OFFSET] and [RIGHT_MODULE_OFFSET] eye character from font data
        uint16_t timeFrame;  // time for this frame in milliseconds
    };

    // Define an entry in the animation sequence lookup table
    struct animTable_t {
        emotion_t e;
        const animFrame_t* seq;
        uint8_t size;
    };

    // Display parameters
    MD_MAX72XX* _M;
    State _state;
    uint16_t _sd;  // start module for the display

    // Animation parameters
    uint32_t _timeStartPause;
    uint32_t _timeLastAnimation;
    uint16_t _timeBlinkMinimum;
    animState_t _animState;
    bool _autoBlink;
    uint16_t _scrollDelay; // This variable seems unused in the provided code

    // Animation control data
    animTable_t _animEntry;     // record with animation sequence parameters
    int8_t _animIndex;          // current index in the animation sequence
    bool _animReverse;          // true = reverse sequence, false = normal sequence
    bool _autoReverse;          // true = always play the reverse, false = selected direction only
    emotion_t _nextEmotion;     // the next emotion to display
    emotion_t _currentEmotion;  // the current emotion being displayed
    emotion_t _idleEmotion;     // the emotion to display when idle // This variable seems unused in the provided code
    const char* _pText;         // pointer to text data in user code. Not nullptr means there is text to print

    // Methods
    void loadEye(uint8_t module, uint8_t ch);
    void drawEyes(uint8_t L, uint8_t R);
    uint8_t loadSequence(emotion_t e);  // return the size of the sequence
    void loadFrame(animFrame_t* pBuf);
    void showText(bool bInit = false);

    // Debugging routine only (commented out as DEBUG is 0)
    // void dumpSequence(const animFrame_t* pBuf, uint8_t numElements);

    // Static data tables - Declarations (defined below)
    static const animFrame_t seqBlink[], seqWink[];
    static const animFrame_t seqLeft[], seqRight[], seqUp[], seqDown[];
    static const animFrame_t seqAngry[], seqSad[], seqEvil[], seqEvil2[];
    static const animFrame_t seqSquint[], seqDead[], seqSquintBlink[], seqIdle[];
    static const animFrame_t seqScanUpDown[], seqScanLeftRight[];
    static const animFrame_t seqHeart[], seqSleep[];

    // Lookup table to find animation - Declaration (defined below)
    static const animTable_t lookupTable[];
};

// Define MD_MAX72XX globally as used in Persona constructor
MD_MAX72XX M = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// EmotiveEye class static variables (from RobotEyes_Data.h)
// Sequences for animations
const Persona::animFrame_t Persona::seqBlink[] PROGMEM =
    {
        {{0, 0}, FRAME_TIME / 2},
        {{1, 1}, FRAME_TIME / 2},
        {{2, 2}, FRAME_TIME / 2},
        {{3, 3}, FRAME_TIME / 2},
        {{4, 4}, FRAME_TIME / 2},
        {{5, 5}, FRAME_TIME},
};

const Persona::animFrame_t Persona::seqWink[] PROGMEM =
    {
        {{0, 0}, FRAME_TIME / 2},
        {{1, 0}, FRAME_TIME / 2},
        {{2, 0}, FRAME_TIME / 2},
        {{3, 0}, FRAME_TIME / 2},
        {{4, 0}, FRAME_TIME / 2},
        {{5, 0}, FRAME_TIME * 2},
};

const Persona::animFrame_t Persona::seqRight[] PROGMEM =
    {
        {{0, 0}, FRAME_TIME},
        {{6, 6}, FRAME_TIME},
        {{7, 7}, FRAME_TIME * 5},
};

const Persona::animFrame_t Persona::seqLeft[] PROGMEM =
    {
        {{0, 0}, FRAME_TIME},
        {{8, 8}, FRAME_TIME},
        {{9, 9}, FRAME_TIME * 5},
};

const Persona::animFrame_t Persona::seqUp[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{11, 11}, FRAME_TIME},
        {{12, 12}, FRAME_TIME},
        {{13, 13}, FRAME_TIME * 5},
};

const Persona::animFrame_t Persona::seqDown[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{14, 14}, FRAME_TIME},
        {{15, 15}, FRAME_TIME},
        {{16, 16}, FRAME_TIME * 5},
};

const Persona::animFrame_t Persona::seqAngry[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{22, 17}, FRAME_TIME},
        {{23, 18}, FRAME_TIME},
        {{24, 19}, FRAME_TIME},
        {{25, 20}, 2000},
};

const Persona::animFrame_t Persona::seqSad[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{32, 27}, FRAME_TIME},
        {{33, 28}, FRAME_TIME},
        {{34, 29}, 2000},
};

const Persona::animFrame_t Persona::seqEvil[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{39, 37}, FRAME_TIME},
        {{40, 38}, 2000},
};

const Persona::animFrame_t Persona::seqEvil2[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{54, 17}, FRAME_TIME},
        {{55, 18}, FRAME_TIME},
        {{56, 19}, FRAME_TIME},
        {{57, 20}, 2000},
};

const Persona::animFrame_t Persona::seqSquint[] PROGMEM =
    {
        {{00, 00}, FRAME_TIME},
        {{54, 54}, FRAME_TIME},
        {{55, 55}, FRAME_TIME},
        {{56, 56}, FRAME_TIME},
        {{57, 57}, 2000},
};

const Persona::animFrame_t Persona::seqDead[] PROGMEM =
    {
        {{52, 52}, FRAME_TIME * 4},
        {{53, 53}, FRAME_TIME * 4},
        {{52, 52}, FRAME_TIME * 2},
};

const Persona::animFrame_t Persona::seqScanLeftRight[] PROGMEM =
    {
        {{41, 41}, FRAME_TIME * 2},
        {{42, 42}, FRAME_TIME},
        {{43, 43}, FRAME_TIME},
        {{44, 44}, FRAME_TIME},
};

const Persona::animFrame_t Persona::seqScanUpDown[] PROGMEM = {
    {{46, 46}, FRAME_TIME * 2},
    {{47, 47}, FRAME_TIME},
    {{48, 48}, FRAME_TIME},
    {{49, 49}, FRAME_TIME},
    {{50, 50}, FRAME_TIME},
    {{51, 51}, FRAME_TIME},
};

const Persona::animFrame_t Persona::seqSquintBlink[] PROGMEM = {
    {{57, 57}, FRAME_TIME},
    {{5, 5}, FRAME_TIME},
};

const Persona::animFrame_t Persona::seqIdle[] PROGMEM = {
    {{57, 57}, FRAME_TIME},
};

const Persona::animFrame_t Persona::seqHeart[] PROGMEM = {
    {{0, 0}, FRAME_TIME},
    {{59, 59}, FRAME_TIME},
    {{60, 60}, 2000},
};

const Persona::animFrame_t Persona::seqSleep[] PROGMEM = {
    {{00, 00}, FRAME_TIME},
    {{62, 62}, FRAME_TIME},
    {{63, 63}, FRAME_TIME},
    {{64, 64}, FRAME_TIME},
    {{65, 65}, FRAME_TIME},
    {{66, 66}, FRAME_TIME},
    {{67, 67}, FRAME_TIME},
};

// Lookup table to find animation sequences
// Table associates the data for an emotion with the sequence table and it's size
const Persona::animTable_t Persona::lookupTable[] PROGMEM = {
    {Persona::E_NEUTRAL, Persona::seqBlink, 1},  // special case, fixed neutral stare
    {Persona::E_BLINK, Persona::seqBlink, ARRAY_SIZE(Persona::seqBlink)},
    {Persona::E_WINK, Persona::seqWink, ARRAY_SIZE(Persona::seqWink)},
    {Persona::E_LOOK_L, Persona::seqLeft, ARRAY_SIZE(Persona::seqLeft)},
    {Persona::E_LOOK_R, Persona::seqRight, ARRAY_SIZE(Persona::seqRight)},
    {Persona::E_LOOK_U, Persona::seqUp, ARRAY_SIZE(Persona::seqUp)},
    {Persona::E_LOOK_D, Persona::seqDown, ARRAY_SIZE(Persona::seqDown)},
    {Persona::E_ANGRY, Persona::seqAngry, ARRAY_SIZE(Persona::seqAngry)},
    {Persona::E_SAD, Persona::seqSad, ARRAY_SIZE(Persona::seqSad)},
    {Persona::E_EVIL, Persona::seqEvil, ARRAY_SIZE(Persona::seqEvil)},
    {Persona::E_EVIL2, Persona::seqEvil2, ARRAY_SIZE(Persona::seqEvil2)},
    {Persona::E_DEAD, Persona::seqDead, ARRAY_SIZE(Persona::seqDead)},
    {Persona::E_SCAN_LR, Persona::seqScanLeftRight, ARRAY_SIZE(Persona::seqScanLeftRight)},
    {Persona::E_SCAN_UD, Persona::seqScanUpDown, ARRAY_SIZE(Persona::seqScanUpDown)},
    // Idle animations
    {Persona::E_IDLE, Persona::seqIdle, ARRAY_SIZE(Persona::seqIdle)},
    {Persona::E_SQUINT, Persona::seqSquint, ARRAY_SIZE(Persona::seqSquint)},
    {Persona::E_SQUINT_BLINK, Persona::seqSquintBlink, ARRAY_SIZE(Persona::seqSquintBlink)},
    // Emojis
    {Persona::E_HEART, Persona::seqHeart, ARRAY_SIZE(Persona::seqHeart)},
    // Sleep
    {Persona::E_SLEEP, Persona::seqSleep, ARRAY_SIZE(Persona::seqSleep)},
};

// Font file (bitmaps for emotion animation frames) (from RobotEyes_Data.h)
MD_MAX72XX::fontType_t _RobotEyes_Font[] PROGMEM =
    {
        8, 0, 126, 129, 177, 177, 129, 126, 0,      // 0   - 'Rest Position'
        8, 0, 124, 130, 178, 178, 130, 124, 0,      // 1   - 'Blink 1'
        8, 0, 120, 132, 180, 180, 132, 120, 0,      // 2   - 'Blink 2'
        8, 0, 48, 72, 120, 120, 72, 48, 0,          // 3   - 'Blink 3'
        8, 0, 32, 80, 112, 112, 80, 32, 0,          // 4   - 'Blink 4'
        8, 0, 32, 96, 96, 96, 96, 32, 0,            // 5   - 'Blink 5'
        8, 0, 126, 129, 129, 177, 177, 126, 0,      // 6   - 'Right 1'
        8, 0, 0, 126, 129, 129, 177, 177, 126,      // 7   - 'Right 2'
        8, 0, 126, 177, 177, 129, 129, 126, 0,      // 8   - 'Left 1'
        8, 126, 177, 177, 129, 129, 126, 0, 0,      // 9   - 'Left 2'
        0,                                          // 10
        8, 0, 126, 129, 153, 153, 129, 126, 0,      // 11   - 'Up 1'
        8, 0, 126, 129, 141, 141, 129, 126, 0,      // 12   - 'Up 2'
        8, 0, 126, 129, 135, 135, 129, 126, 0,      // 13   - 'Up 3'
        8, 0, 126, 129, 225, 225, 129, 126, 0,      // 14   - 'Down 1'
        8, 0, 126, 129, 193, 193, 129, 126, 0,      // 15   - 'Down 2'
        8, 0, 124, 130, 194, 194, 130, 124, 0,      // 16   - 'Down 3'
        8, 0, 124, 130, 177, 177, 129, 126, 0,      // 17   - 'Angry L 1'
        8, 0, 120, 132, 178, 177, 129, 126, 0,      // 18   - 'Angry L 2'
        8, 0, 112, 136, 164, 178, 129, 126, 0,      // 19   - 'Angry L 3'
        8, 0, 96, 144, 168, 180, 130, 127, 0,       // 20   - 'Angry L 4'
        0,                                          // 21
        8, 0, 126, 129, 177, 177, 130, 124, 0,      // 22   - 'Angry R 1'
        8, 0, 126, 129, 177, 178, 132, 120, 0,      // 23   - 'Angry R 2'
        8, 0, 126, 129, 178, 164, 136, 112, 0,      // 24   - 'Angry R 3'
        8, 0, 127, 130, 180, 168, 144, 96, 0,       // 25   - 'Angry R 4'
        0,                                          // 26
        8, 0, 62, 65, 153, 153, 130, 124, 0,        // 27   - 'Sad L 1'
        8, 0, 30, 33, 89, 154, 132, 120, 0,         // 28   - 'Sad L 2'
        8, 0, 14, 17, 41, 90, 132, 120, 0,          // 29   - 'Sad L 3'
        0,                                          // 30
        0,                                          // 31
        8, 0, 124, 130, 153, 153, 65, 62, 0,        // 32   - 'Sad R 1'
        8, 0, 120, 132, 154, 89, 33, 30, 0,         // 33   - 'Sad R 2'
        8, 0, 120, 132, 90, 41, 17, 14, 0,          // 34   - 'Sad R 3'
        0,                                          // 35
        0,                                          // 36
        8, 0, 124, 194, 177, 177, 193, 126, 0,      // 37   - 'Evil L 1'
        8, 0, 56, 68, 178, 177, 66, 60, 0,          // 38   - 'Evil L 2'
        8, 0, 126, 193, 177, 177, 194, 124, 0,      // 39   - 'Evil R 1'
        8, 0, 60, 66, 177, 178, 68, 56, 0,          // 40   - 'Evil R 2'
        8, 0, 126, 129, 129, 129, 189, 126, 0,      // 41   - 'Scan H 1'
        8, 0, 126, 129, 129, 189, 129, 126, 0,      // 42   - 'Scan H 2'
        8, 0, 126, 129, 189, 129, 129, 126, 0,      // 43   - 'Scan H 3'
        8, 0, 126, 189, 129, 129, 129, 126, 0,      // 44   - 'Scan H 4'
        0,                                          // 45
        8, 0, 126, 129, 131, 131, 129, 126, 0,      // 46   - 'Scan V 1'
        8, 0, 126, 129, 133, 133, 129, 126, 0,      // 47   - 'Scan V 2'
        8, 0, 126, 129, 137, 137, 129, 126, 0,      // 48   - 'Scan V 3'
        8, 0, 126, 129, 145, 145, 129, 126, 0,      // 49   - 'Scan V 4'
        8, 0, 126, 129, 161, 161, 129, 126, 0,      // 50   - 'Scan V 5'
        8, 0, 126, 129, 193, 193, 129, 126, 0,      // 51   - 'Scan V 6'
        8, 0, 126, 137, 157, 137, 129, 126, 0,      // 52   - 'RIP 1'
        8, 0, 126, 129, 145, 185, 145, 126, 0,      // 53   - 'RIP 2'
        8, 0, 60, 66, 114, 114, 66, 60, 0,          // 54   - 'Peering 1'
        8, 0, 56, 68, 116, 116, 68, 56, 0,          // 55   - 'Peering 2'
        8, 0, 48, 72, 120, 120, 72, 48, 0,          // 56   - 'Peering 3'
        8, 0, 32, 80, 112, 112, 80, 32, 0,          // 57   - 'Peering 4'  Idle position reset
        0,                                          // 58
        8, 0, 28, 34, 114, 114, 34, 28, 0,          // 59   - Core 1
        8, 0, 28, 34, 100, 100, 34, 28, 0,          // 60   - Core 2
        0,                                          // 61   - 'Unused'
        8, 129, 193, 161, 145, 137, 133, 131, 129,  // 62   - 'Sleep 1'
        8, 144, 208, 176, 144, 9, 13, 11, 9,        // 63   - 'Sleep 2'
        8, 160, 224, 160, 20, 28, 20, 0, 0,         // 64   - 'Sleep 3'
        8, 144, 208, 176, 144, 0, 0, 0, 0,          // 65   - 'Sleep 4'
        8, 160, 224, 160, 0, 0, 0, 0, 0,            // 66   - 'Sleep 5'
        8, 128, 0, 0, 0, 0, 0, 0, 0,                // 67   - 'Sleep 6'
        0,                                          // 68 - 'Unused'
        0,                                          // 69 - 'Unused'
        0,                                          // 70 - 'Unused'
        0,                                          // 71 - 'Unused'
        0,                                          // 72 - 'Unused'
        0,                                          // 73 - 'Unused'
        0,                                          // 74 - 'Unused'
        0,                                          // 75 - 'Unused'
        0,                                          // 76 - 'Unused'
        0,                                          // 77 - 'Unused'
        0,                                          // 78 - 'Unused'
        0,                                          // 79 - 'Unused'
        0,                                          // 80 - 'Unused'
        0,                                          // 81 - 'Unused'
        0,                                          // 82 - 'Unused'
        0,                                          // 83 - 'Unused'
        0,                                          // 84 - 'Unused'
        0,                                          // 85 - 'Unused'
        0,                                          // 86 - 'Unused'
        0,                                          // 87 - 'Unused'
        0,                                          // 88 - 'Unused'
        0,                                          // 89 - 'Unused'
        0,                                          // 90 - 'Unused'
        0,                                          // 91 - 'Unused'
        0,                                          // 92 - 'Unused'
        0,                                          // 93 - 'Unused'
        0,                                          // 94 - 'Unused'
        0,                                          // 95 - 'Unused'
        0,                                          // 96 - 'Unused'
        0,                                          // 97 - 'Unused'
        0,                                          // 98 - 'Unused'
        0,                                          // 99 - 'Unused'
        0,                                          // 100 - 'Unused'
        0,                                          // 101 - 'Unused'
        0,                                          // 102 - 'Unused'
        0,                                          // 103 - 'Unused'
        0,                                          // 104 - 'Unused'
        0,                                          // 105 - 'Unused'
        0,                                          // 106 - 'Unused'
        0,                                          // 107 - 'Unused'
        0,                                          // 108 - 'Unused'
        0,                                          // 109 - 'Unused'
        0,                                          // 110 - 'Unused'
        0,                                          // 111 - 'Unused'
        0,                                          // 112 - 'Unused'
        0,                                          // 113 - 'Unused'
        0,                                          // 114 - 'Unused'
        0,                                          // 115 - 'Unused'
        0,                                          // 116 - 'Unused'
        0,                                          // 117 - 'Unused'
        0,                                          // 118 - 'Unused'
        0,                                          // 119 - 'Unused'
        0,                                          // 120 - 'Unused'
        0,                                          // 121 - 'Unused'
        0,                                          // 122 - 'Unused'
        0,                                          // 123 - 'Unused'
        0,                                          // 124 - 'Unused'
        0,                                          // 125 - 'Unused'
        0,                                          // 126 - 'Unused'
        0,                                          // 127 - 'Unused'
        0,                                          // 128 - 'Unused'
        0,                                          // 129 - 'Unused'
        0,                                          // 130 - 'Unused'
        0,                                          // 131 - 'Unused'
        0,                                          // 132 - 'Unused'
        0,                                          // 133 - 'Unused'
        0,                                          // 134 - 'Unused'
        0,                                          // 135 - 'Unused'
        0,                                          // 136 - 'Unused'
        0,                                          // 137 - 'Unused'
        0,                                          // 138 - 'Unused'
        0,                                          // 139 - 'Unused'
        0,                                          // 140 - 'Unused'
        0,                                          // 141 - 'Unused'
        0,                                          // 142 - 'Unused'
        0,                                          // 143 - 'Unused'
        0,                                          // 144 - 'Unused'
        0,                                          // 145 - 'Unused'
        0,                                          // 146 - 'Unused'
        0,                                          // 147 - 'Unused'
        0,                                          // 148 - 'Unused'
        0,                                          // 149 - 'Unused'
        0,                                          // 150 - 'Unused'
        0,                                          // 151 - 'Unused'
        0,                                          // 152 - 'Unused'
        0,                                          // 153 - 'Unused'
        0,                                          // 154 - 'Unused'
        0,                                          // 155 - 'Unused'
        0,                                          // 156 - 'Unused'
        0,                                          // 157 - 'Unused'
        0,                                          // 158 - 'Unused'
        0,                                          // 159 - 'Unused'
        0,                                          // 160 - 'Unused'
        0,                                          // 161 - 'Unused'
        0,                                          // 162 - 'Unused'
        0,                                          // 163 - 'Unused'
        0,                                          // 164 - 'Unused'
        0,                                          // 165 - 'Unused'
        0,                                          // 166 - 'Unused'
        0,                                          // 167 - 'Unused'
        0,                                          // 168 - 'Unused'
        0,                                          // 169 - 'Unused'
        0,                                          // 170 - 'Unused'
        0,                                          // 171 - 'Unused'
        0,                                          // 172 - 'Unused'
        0,                                          // 173 - 'Unused'
        0,                                          // 174 - 'Unused'
        0,                                          // 175 - 'Unused'
        0,                                          // 176 - 'Unused'
        0,                                          // 177 - 'Unused'
        0,                                          // 178 - 'Unused'
        0,                                          // 179 - 'Unused'
        0,                                          // 180 - 'Unused'
        0,                                          // 181 - 'Unused'
        0,                                          // 182 - 'Unused'
        0,                                          // 183 - 'Unused'
        0,                                          // 184 - 'Unused'
        0,                                          // 185 - 'Unused'
        0,                                          // 186 - 'Unused'
        0,                                          // 187 - 'Unused'
        0,                                          // 188 - 'Unused'
        0,                                          // 189 - 'Unused'
        0,                                          // 190 - 'Unused'
        0,                                          // 191 - 'Unused'
        0,                                          // 192 - 'Unused'
        0,                                          // 193 - 'Unused'
        0,                                          // 194 - 'Unused'
        0,                                          // 195 - 'Unused'
        0,                                          // 196 - 'Unused'
        0,                                          // 197 - 'Unused'
        0,                                          // 198 - 'Unused'
        0,                                          // 199 - 'Unused'
        0,                                          // 200 - 'Unused'
        0,                                          // 201 - 'Unused'
        0,                                          // 202 - 'Unused'
        0,                                          // 203 - 'Unused'
        0,                                          // 204 - 'Unused'
        0,                                          // 205 - 'Unused'
        0,                                          // 206 - 'Unused'
        0,                                          // 207 - 'Unused'
        0,                                          // 208 - 'Unused'
        0,                                          // 209 - 'Unused'
        0,                                          // 210 - 'Unused'
        0,                                          // 211 - 'Unused'
        0,                                          // 212 - 'Unused'
        0,                                          // 213 - 'Unused'
        0,                                          // 214 - 'Unused'
        0,                                          // 215 - 'Unused'
        0,                                          // 216 - 'Unused'
        0,                                          // 217 - 'Unused'
        0,                                          // 218 - 'Unused'
        0,                                          // 219 - 'Unused'
        0,                                          // 220 - 'Unused'
        0,                                          // 221 - 'Unused'
        0,                                          // 222 - 'Unused'
        0,                                          // 223 - 'Unused'
        0,                                          // 224 - 'Unused'
        0,                                          // 225 - 'Unused'
        0,                                          // 226 - 'Unused'
        0,                                          // 227 - 'Unused'
        0,                                          // 228 - 'Unused'
        0,                                          // 229 - 'Unused'
        0,                                          // 230 - 'Unused'
        0,                                          // 231 - 'Unused'
        0,                                          // 232 - 'Unused'
        0,                                          // 233 - 'Unused'
        0,                                          // 234 - 'Unused'
        0,                                          // 235 - 'Unused'
        0,                                          // 236 - 'Unused'
        0,                                          // 237 - 'Unused'
        0,                                          // 238 - 'Unused'
        0,                                          // 239 - 'Unused'
        0,                                          // 240 - 'Unused'
        0,                                          // 241 - 'Unused'
        0,                                          // 242 - 'Unused'
        0,                                          // 243 - 'Unused'
        0,                                          // 244 - 'Unused'
        0,                                          // 245 - 'Unused'
        0,                                          // 246 - 'Unused'
        0,                                          // 247 - 'Unused'
        0,                                          // 248 - 'Unused'
        0,                                          // 249 - 'Unused'
        0,                                          // 250 - 'Unused'
        0,                                          // 251 - 'Unused'
        0,                                          // 252 - 'Unused'
        0,                                          // 253 - 'Unused'
        0,                                          // 254 - 'Unused'
        0,                                          // 255
};

// Persona class method implementations (from persona.cpp)

// Debugging macros (set DEBUG to 0 to disable prints)
#define DEBUG 0

#if DEBUG
#define PRINTS(s) \
    { Serial.print(F(s)); }
#define PRINT(s, v)         \
    {                       \
        Serial.print(F(s)); \
        Serial.print(v);    \
    }
#define PRINTX(s, v)           \
    {                          \
        Serial.print(F(s));    \
        Serial.print(F("0x")); \
        Serial.print(v, HEX);  \
    }
#else
#define PRINTS(s)
#define PRINT(s, v)
#define PRINTX(s, v)
#endif


Persona::Persona(void) : _timeBlinkMinimum(5000), _animState(S_IDLE), _autoBlink(true), _nextEmotion(E_NEUTRAL) {};

void Persona::loadEye(uint8_t module, uint8_t ch) {
    uint8_t buf[EYE_COL_SIZE];
    // Assuming getChar exists and is accessible from MD_MAX72xx M
    uint8_t size = M.getChar(ch, EYE_COL_SIZE, buf);

    for (uint8_t i = 0; i < size; i++) {
        M.setColumn(module, i, buf[i]);
    }
}

void Persona::drawEyes(uint8_t L, uint8_t R)
// Draw the left and right eyes
{
    // Assuming getFont, control, clear, setFont are accessible from MD_MAX72xx M
    MD_MAX72XX::fontType_t* savedFont = M.getFont();

    M.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    M.setFont(_RobotEyes_Font);

    M.clear(_sd, _sd + 1);  // clear out display modules

    // Load the data and show it
    loadEye(_sd + LEFT_MODULE_OFFSET, L);
    loadEye(_sd + RIGHT_MODULE_OFFSET, R);

    M.setFont(savedFont);
    M.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

#if DEBUG
// Debugging routine to display an animation table in PROGMEM
// void Persona::dumpSequence(const animFrame_t* pBuf, uint8_t numElements)
// {
//     for (uint8_t i = 0; i < numElements; i++) {
//         animFrame_t f;

//         memcpy_P(&f, &pBuf[i], sizeof(animFrame_t));
//         PRINT("\n[", i);
//         PRINT("]: L:", f.eyeData[LEFT_EYE_INDEX]);
//         PRINT(" R:", f.eyeData[RIGHT_EYE_INDEX]);
//         PRINT(" T:", f.timeFrame);
//     }
// }
#endif

uint8_t Persona::loadSequence(emotion_t e)
// Load the next emotion from the static data.
// Set global variables to the required values
{
    // run through the lookup table to find the sequence data
    for (uint8_t i = 0; i < ARRAY_SIZE(lookupTable); i++) {
        animTable_t entry; // Create a local copy
        memcpy_P(&entry, &lookupTable[i], sizeof(animTable_t));
        if (entry.e == e) {
            _animEntry = entry; // Assign the found entry to the member variable
#if DEBUG
            // dumpSequence(_animEntry.seq, _animEntry.size);
#endif
            break;
        }
    }

    // set up the current index depending on direction of animation
    if (_animReverse)
        _animIndex = _animEntry.size - 1;
    else
        _animIndex = 0;

    return (_animEntry.size);
}

void Persona::loadFrame(animFrame_t* pBuf)
// Load the idx'th frame from the frame sequence PROGMEM to normal memory pBuf
{
    memcpy_P(pBuf, &_animEntry.seq[_animIndex], sizeof(animFrame_t));
}

void Persona::showText(bool bInit)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
    static enum { S_LOAD,
                  S_SHOW,
                  S_SPACE } state;
    static uint8_t curLen, showLen;
    static uint8_t cBuf[EYE_COL_SIZE];

    if (bInit) {
        PRINT("\nText: ", _pText);
        _timeLastAnimation = millis();
        M.clear(_sd, _sd + 1);
        state = S_LOAD;
    }

    // Is it time to scroll the text?
    if (millis() - _timeLastAnimation < FRAME_TIME / 2)
        return;

    M.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

    // Now scroll the text
    M.transform(_sd, _sd + 1, MD_MAX72XX::TSL);  // scroll along by one place
    _timeLastAnimation = millis();                 // starting time for next scroll

    // Now work out what's next using finite state machine to control what we do
    switch (state) {
        case S_LOAD:  // Load the next character from the font table
            // if we reached end of message or empty string, reset the message pointer
            if (*_pText == '\0') {
                // Free the duplicated string if it was allocated by strdup
                // Note: This simple implementation assumes strdup was used for setText.
                // In a more robust version, you'd need to track if _pText was allocated
                // dynamically and free it appropriately. For this integrated example,
                // we'll skip explicit free for simplicity, but be aware of potential leak.
                // if (_pText != nullptr) free((void*)_pText); // Example
                _pText = nullptr;
                break;
            }

            // otherwise load the character
            // Assuming getChar is accessible from MD_MAX72xx M
            showLen = M.getChar(*_pText++, ARRAY_SIZE(cBuf), cBuf);
            curLen = 0;
            state = S_SHOW;
            // fall through to the next state

        case S_SHOW:  // display the next part of the character
             // Assuming setColumn is accessible from MD_MAX72xx M
            M.setColumn(_sd, 0, cBuf[curLen++]);
            if (curLen == showLen) {
                showLen = (*_pText == '\0' ? 2 * EYE_COL_SIZE : 1);  // either 1 space or pad to the end of the display if finished
                curLen = 0;
                state = S_SPACE;
            }
            break;

        case S_SPACE:  // display inter-character spacing (blank columns)
             // Assuming setColumn is accessible from MD_MAX72xx M
            M.setColumn(_sd, 0, 0);
            curLen++;
            if (curLen >= showLen)
                state = S_LOAD;
            break;

        default:
            state = S_LOAD;
    }

    M.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void Persona::setState(State s) {
    if (s != _state) {
        if (s == SLEEPING && _state == AWAKE) {
            setAnimation(E_SQUINT, false, false, true);
        } else if (s == AWAKE && _state == SLEEPING) {
            setAnimation(E_SQUINT, false, true, true);
        }
        _state = s;
        PRINT("\nState: ", _state);
    }
}

void Persona::begin(uint8_t moduleStart)
// initialize other stuff after libraries have started
{
    PRINTS("\n[Persona Debug]");
    // M.begin() is called in the global setup() now
    _M = &M; // Assign the global M instance
    _sd = moduleStart;
    _state = AWAKE;

    setAnimation(E_NEUTRAL, false);
};

bool Persona::runAnimation(void)
// Animate the eyes
// Return true if there is no animation happening
{
    static animFrame_t thisFrame;
    static uint32_t timeOfLastFrame = 0; // Track time for frame pauses

    switch (_animState) {
        case S_IDLE:                // no animation running - wait for a new one or blink if time to do so
            if (_pText != nullptr)  // there is some text to show
            {
                PRINTS("\nIDLE: showing text");
                showText(true);
                _animState = S_TEXT;
                timeOfLastFrame = millis(); // Start text scroll timer
                break;
            }

            if (_nextEmotion != E_NONE) // Check if there's a pending emotion change
            {
                 PRINTS("\nIDLE: Emotion change pending");
                 _animState = S_RESTART; // Go to restart to handle the next emotion
                 break;
            }

            // otherwise fall through and try for auto blink
            if (_autoBlink)  // check if we should be blinking
            {
                if (((millis() - _timeLastAnimation) >= _timeBlinkMinimum) && (random(1000) > 700)) {
                    PRINTS("\nIDLE: Forcing blink");
                    if (_state == SLEEPING) {
                        setAnimation(E_SQUINT_BLINK, true);
                    } else if (_state == AWAKE) {
                        setAnimation(E_BLINK, true);
                    }
                    // After setting the animation, the state will transition to S_RESTART in the next loop iteration
                    _timeLastAnimation = millis(); // Reset blink timer after starting blink
                }
            }
            break;

        case S_RESTART:                  // back to start of current animation
            if (_nextEmotion != E_NONE)  // check if we have an animation in the queue
            {
                PRINTS("\nRESRT: showing animation");
                _timeLastAnimation = millis(); // Reset animation timer

                // set up the next animation
                loadSequence(_nextEmotion);
                _currentEmotion = _nextEmotion;
                _nextEmotion = E_NONE;
                _animState = S_ANIMATE;
            } else {
                 // Should not reach here if _nextEmotion is NONE unless called directly with S_RESTART state,
                 // or if setAnimation was called but _nextEmotion was NONE.
                 // Return to IDLE if no animation is queued.
                 _animState = S_IDLE;
            }
            break;

        case S_ANIMATE:  // process the next frame for this sequence
            PRINT("\nPROCESS: Frame:", _animIndex);
            loadFrame(&thisFrame);
            drawEyes(thisFrame.eyeData[LEFT_EYE_INDEX], thisFrame.eyeData[RIGHT_EYE_INDEX]);
            if (_animReverse) {
                _animIndex--;
            } else {
                _animIndex++;
            }

            timeOfLastFrame = millis(); // Record time when this frame was displayed
            _animState = S_PAUSE;
            break;

        case S_PAUSE:  // pause this frame for the required time
        {
            if ((millis() - timeOfLastFrame) < thisFrame.timeFrame) {
                break; // Not enough time has passed
            }

            // check if this is the end of animation
            if ((!_animReverse && _animIndex >= _animEntry.size) || (_animReverse && _animIndex < 0)) {
                PRINTS("\nPAUSE: Animation end")
                if (_autoReverse)  // set up the same emotion but in reverse
                {
                    PRINTS(" & auto reverse");
                    _nextEmotion = _animEntry.e;
                    _animReverse = true;   // set this flag for the restart state
                    _autoReverse = false;  // clear the flag for this animation sequence
                    _animState = S_RESTART;
                } else {
                    _animState = S_IDLE; // Animation finished, return to idle
                    _currentEmotion = E_NONE; // Clear current emotion
                }
            } else {
                _animState = S_ANIMATE; // Move to the next frame
            }
        } break;

        case S_TEXT:  // currently displaying text
        {
            showText();
            if (_pText == nullptr) {
                _animState = S_IDLE; // Text finished, return to idle
                _timeLastAnimation = millis(); // Reset blink timer after text
            }
        } break;

        default:  // something is wrong - reset the FSM
            _animState = S_IDLE;
            _currentEmotion = E_NONE;
            _pText = nullptr;
            _timeLastAnimation = millis(); // Reset blink timer on error
            break;
    }

    return (_animState == S_IDLE); // Return true if currently in IDLE state
};

void Persona::processCommand(const char* command) {
    // This function sets the next animation based on a command string.
    // The actual animation change happens in runAnimation()'s S_IDLE or S_RESTART state
    // when _nextEmotion is processed.
    if (strcmp(command, "neutral") == 0) {
        setAnimation(Persona::E_NEUTRAL, true, false, true);
    } else if (strcmp(command, "blink") == 0) {
        setAnimation(Persona::E_BLINK, true, false, true);
    } else if (strcmp(command, "angry") == 0) {
        setAnimation(Persona::E_ANGRY, true, false, true);
    } else if (strcmp(command, "sad") == 0) {
        setAnimation(Persona::E_SAD, true, false, true);
    } else if (strcmp(command, "evil") == 0) {
        setAnimation(Persona::E_EVIL, true, false, true);
    } else if (strcmp(command, "evil2") == 0) {
        setAnimation(Persona::E_EVIL2, true, false, true);
    } else if (strcmp(command, "squint") == 0) {
        setAnimation(Persona::E_SQUINT, true, false, true);
    } else if (strcmp(command, "dead") == 0) {
        setAnimation(Persona::E_DEAD, true, false, true);
    } else if (strcmp(command, "core") == 0) { // Assuming "core" command corresponds to E_HEART
        setAnimation(Persona::E_HEART, true, false, true);
    } else if (strcmp(command, "sleep") == 0) {
        setAnimation(Persona::E_SLEEP, true, false, true);
    }
    // Handle state changes (AWAKE/SLEEPING) via processCommand if needed,
    // although setState is specifically for the AWAKE/SLEEPING logic.
    // Keeping setState for the original logic based on TIME_TO_SLEEP.
    /*
    else if (strcmp(command, "awake") == 0) {
        setState(AWAKE);
    } else if (strcmp(command, "sleeping") == 0) {
        setState(SLEEPING);
    }
    */
    // Text in display
    else {
        // Note: strdup allocates memory that should ideally be freed.
        // In this single-file example, we're simplifying.
        // In a real application, manage this memory carefully.
        setText(strdup(command));
    }
}


// Global Persona instance
Persona persona;

// Example variables for state management (similar to original main.cpp)
unsigned long lastCommandTime = 0; // Example: track time of last command to change state

// Setup function
void setup() {
    Serial.begin(115200);
    Serial.println("Persona integration example");

    // Initialize the MD_MAX72xx display
    M.begin();

    // Initialize the Persona object
    persona.begin();

    lastCommandTime = millis(); // Initialize last command time
}

// Loop function
void loop() {
    // Run the persona animation logic
    persona.runAnimation();

    // Example state change logic based on time (similar to original main.cpp)
    // Assuming a simple state change after a period of "inactivity"
    const unsigned long TIME_TO_SLEEP_EXAMPLE = 10000; // 10 seconds for example

    if (millis() - lastCommandTime >= TIME_TO_SLEEP_EXAMPLE) {
        persona.setState(SLEEPING);
    } else {
        persona.setState(AWAKE);
    }

    // Simple Serial input processing for commands (optional)
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim(); // Remove newline and whitespace
        Serial.print("Received command: ");
        Serial.println(command);
        persona.processCommand(command.c_str());
        lastCommandTime = millis(); // Reset timer on receiving a command
    }

    // Add a small delay to prevent the loop from running too fast if needed
    // delay(1);
}
