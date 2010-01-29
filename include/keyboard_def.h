#ifndef KEYBOARD_DEF_H
#define KEYBOARD_DEF_H

#define KEYBOARD_BUFFER_SIZE 64
#define ARROW_UP_KEY 256
#define ARROW_DOWN_KEY 257
#define ARROW_RIGHT_KEY 258
#define ARROW_LEFT_KEY 259
#define HOME_KEY 260
#define INSERT_KEY 261
#define DELETE_KEY 262
#define END_KEY 263
#define PAGE_UP_KEY 264
#define PAGE_DOWN_KEY 265
#define PAUSE_KEY 266
#define F1_KEY 267
#define F2_KEY 268
#define F3_KEY 269
#define F4_KEY 270
#define F5_KEY 271
#define F6_KEY 272
#define F7_KEY 273
#define F8_KEY 274
#define F9_KEY 275
#define F10_KEY 276
#define F10_PAD_KEY 277
#define F11_KEY 278
#define F12_KEY 279
#define SHIFT_F1_KEY 280
#define SHIFT_F2_KEY 281
#define SHIFT_F2_PAD_KEY 282
#define SHIFT_F3_KEY 283
#define SHIFT_F4_KEY 284
#define SHIFT_F5_KEY 285
#define SHIFT_F6_KEY 286
#define SHIFT_F7_KEY 287
#define SHIFT_F8_KEY 288

#define NUMERIC_0 45 
#define NUMERIC_1 35 
#define NUMERIC_2 40 
#define NUMERIC_3 34 
#define NUMERIC_4 37 
#define NUMERIC_5 12 
#define NUMERIC_6 39 
#define NUMERIC_7 36 
#define NUMERIC_8 38 
#define NUMERIC_9 33 

#define X_SHIFT_SYM_L 65505
#define X_SHIFT_SYM_R 65505
#define X_F1_SYM 65470
#define X_F2_SYM 65471
#define X_F9_SYM 65478
#define X_F10_SYM 65479
#define X_F11_SYM 65480
#define X_F12_SYM 65481
#define X_ESC_SYM 65307
#define X_HOME_SYM 65360
#define X_PGUP_SYM 65365
#define X_PGDN_SYM 65366
#define X_END_SYM 65367
#define X_PAUSE_SYM 65299
#define X_ENTER_SYM 65293
#define X_UP_SYM 65362
#define X_DWN_SYM 65364
#define X_RIGHT_SYM 65363
#define X_LEFT_SYM 65361
#define X_BACKDEL_SYM 0xff08
#define X_INSERT_SYM 65379
#define X_DELETE_SYM 65535

extern int *keyboard_buffer;
extern int keyboard_buffer_ptr;
extern int keyboard_buffer_used;

#endif
