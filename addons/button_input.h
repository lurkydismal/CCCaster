#if !defined( BUTTON_INPUT_H )
#define BUTTON_INPUT_H

enum button_t {
    NEUTRAL_BUTTON = 0x0,
    A = 0x0010,
    B = 0x0020,
    C = 0x0008,
    D = 0x0004,
    E = 0x0080,
    AB = 0x0040,
    FN1 = 0x0100, // Control dummy
    FN2 = 0x0200, // Training reset
    START = 0x0001,
    CONFIRM = 0x0400,
    CANCEL = 0x0800,
    FACING = 0x0002
};

#endif
