.16bc

.data
	WORD last_key_pressed		0
	WORD keyHandled				0 ; 0 = no key, 1 = key waiting

	WORD RANDOM_NUMBER			1
	WORD ms_passed				0 ; counts timer interrupts

	; keyboard input for the game
	WORD UPKEY					0x11 ; W
	WORD DOWNKEY				0x1F ; S
	WORD LEFTKEY				0x1E ; A
	WORD RIGHTKEY				0x20 ; D

	; 0 = up, 1 = down, 2 = left, 3 = right
	WORD DIRECTION				3
	WORD NEXT_DIRECTION			3

	; 0 = playing, 1 = game over, 2 = won
	WORD GAME_STATE				0
	WORD SCORE					0

	WORD SNAKE_LENGTH			3
	WORD SNAKE_HEAD				0
	WORD SNAKE_TAIL				0

	WORD APPLE_X				0
	WORD APPLE_Y				0

	WORD NEW_X					0
	WORD NEW_Y					0
	WORD EATING					0

	; reserved memory for the snake
	; 1920 segments, each 2 words in size (x, y)
	; circular buffer from 40000 to 43839
	WORD SNAKE_START			40000
	WORD SNAKE_END				43840

	; back buffer from 44000 to 45999
	; front buffer is the display at 63534
	WORD FRAME_START			44000
	WORD FRAME_END				46000

	; SNAKE  SCORE: 0000  WASD MOVE  R RESTART
	WORD HUD_TEXT_00				0x1F53
	WORD HUD_TEXT_01				0x1F4E
	WORD HUD_TEXT_02				0x1F41
	WORD HUD_TEXT_03				0x1F4B
	WORD HUD_TEXT_04				0x1F45
	WORD HUD_TEXT_05				0x1F20
	WORD HUD_TEXT_06				0x1F20
	WORD HUD_TEXT_07				0x1F53
	WORD HUD_TEXT_08				0x1F43
	WORD HUD_TEXT_09				0x1F4F
	WORD HUD_TEXT_10				0x1F52
	WORD HUD_TEXT_11				0x1F45
	WORD HUD_TEXT_12				0x1F3A
	WORD HUD_TEXT_13				0x1F20
	WORD HUD_TEXT_14				0x1F30
	WORD HUD_TEXT_15				0x1F30
	WORD HUD_TEXT_16				0x1F30
	WORD HUD_TEXT_17				0x1F30
	WORD HUD_TEXT_18				0x1F20
	WORD HUD_TEXT_19				0x1F20
	WORD HUD_TEXT_20				0x1F57
	WORD HUD_TEXT_21				0x1F41
	WORD HUD_TEXT_22				0x1F53
	WORD HUD_TEXT_23				0x1F44
	WORD HUD_TEXT_24				0x1F20
	WORD HUD_TEXT_25				0x1F4D
	WORD HUD_TEXT_26				0x1F4F
	WORD HUD_TEXT_27				0x1F56
	WORD HUD_TEXT_28				0x1F45
	WORD HUD_TEXT_29				0x1F20
	WORD HUD_TEXT_30				0x1F20
	WORD HUD_TEXT_31				0x1F52
	WORD HUD_TEXT_32				0x1F20
	WORD HUD_TEXT_33				0x1F52
	WORD HUD_TEXT_34				0x1F45
	WORD HUD_TEXT_35				0x1F53
	WORD HUD_TEXT_36				0x1F54
	WORD HUD_TEXT_37				0x1F41
	WORD HUD_TEXT_38				0x1F52
	WORD HUD_TEXT_39				0x1F54
	WORD HUD_TEXT_END				0

	; PLAYING
	WORD PLAYING_TEXT_00			0x1F50
	WORD PLAYING_TEXT_01			0x1F4C
	WORD PLAYING_TEXT_02			0x1F41
	WORD PLAYING_TEXT_03			0x1F59
	WORD PLAYING_TEXT_04			0x1F49
	WORD PLAYING_TEXT_05			0x1F4E
	WORD PLAYING_TEXT_06			0x1F47
	WORD PLAYING_TEXT_END			0

	; GAME OVER
	WORD GAME_OVER_TEXT_00			0x1F47
	WORD GAME_OVER_TEXT_01			0x1F41
	WORD GAME_OVER_TEXT_02			0x1F4D
	WORD GAME_OVER_TEXT_03			0x1F45
	WORD GAME_OVER_TEXT_04			0x1F20
	WORD GAME_OVER_TEXT_05			0x1F4F
	WORD GAME_OVER_TEXT_06			0x1F56
	WORD GAME_OVER_TEXT_07			0x1F45
	WORD GAME_OVER_TEXT_08			0x1F52
	WORD GAME_OVER_TEXT_END			0

	; YOU WIN
	WORD WIN_TEXT_00				0x1F59
	WORD WIN_TEXT_01				0x1F4F
	WORD WIN_TEXT_02				0x1F55
	WORD WIN_TEXT_03				0x1F20
	WORD WIN_TEXT_04				0x1F57
	WORD WIN_TEXT_05				0x1F49
	WORD WIN_TEXT_06				0x1F4E
	WORD WIN_TEXT_END				0


.code
	; setup the interrupt handler for keyboard input
	mov eax [keyboard_interupt]
	mov 63278 eax

	; set timer interrupt handler
	mov eax [timer_interrupt]
	mov 63280 eax

	; mouse input - vector 1
	mov eax [mouse_interrupt]
	mov 63279 eax

	; errors - vectors 3 through 6
	mov eax [error_interrupt]
	mov 63281 eax ; SegFault
	mov 63282 eax ; Stack Overflow
	mov 63283 eax ; Stack Underflow
	mov 63284 eax ; Divide by Zero

	; interrupt every 1000 instruction ticks
	mov eax 1000
	mov 65535 eax

	; setup the game and draw the screen
	call reset_game
	call render


	; run the snake loop
loop:
	; enable interrupts here
	; disable while handling game data and flags
	setirr 1
	setirr 0

	mov eax keyHandled
	mov ebx 0
	cmp eax ebx
	brz check_timer

	mov keyHandled ebx
	call handle_key


check_timer:
	mov eax GAME_STATE
	mov ebx 0
	cmp eax ebx
	brz check_move

	bra loop


check_move:
	; move once every 30 timer interrupts
	; change this value to adjust the speed
	mov eax ms_passed
	mov ebx 400
	cmp eax ebx
	brn loop

	mov eax 0
	mov ms_passed eax

	call move_snake
	call render

	bra loop


reset_game:
	; reset the game variables
	mov eax 0
	mov GAME_STATE eax
	mov SCORE eax
	mov ms_passed eax
	mov keyHandled eax

	mov eax 3
	mov DIRECTION eax
	mov NEXT_DIRECTION eax
	mov SNAKE_LENGTH eax

	; set snake tail and head
	mov eax SNAKE_START
	mov SNAKE_TAIL eax

	mov ebx 4
	add eax ebx
	mov SNAKE_HEAD eax

	; starting snake segments
	; tail (38, 12), middle (39, 12), head (40, 12)
	mov eax 38
	mov 40000 eax

	mov eax 39
	mov 40002 eax

	mov eax 40
	mov 40004 eax

	mov eax 12
	mov 40001 eax
	mov 40003 eax
	mov 40005 eax

	call place_apple
	ret


; Handle key press
handle_key:
	; ignore key release events
	mov eax last_key_pressed
	mov ebx 0x8000
	and eax ebx

	mov ebx 0
	cmp eax ebx
	brz key_press

	ret


key_press:
	; R restarts the game
	mov eax last_key_pressed
	mov ebx 0x13
	cmp eax ebx
	brz restart_game

	; only change direction while playing
	mov ebx GAME_STATE
	mov ecx 0
	cmp ebx ecx
	brz direction_key

	ret


direction_key:
	; UpKey Handler
	mov ebx UPKEY
	cmp eax ebx
	brz set_up

	; DownKey Handler
	mov ebx DOWNKEY
	cmp eax ebx
	brz set_down

	; LeftKey Handler
	mov ebx LEFTKEY
	cmp eax ebx
	brz set_left

	; RightKey Handler
	mov ebx RIGHTKEY
	cmp eax ebx
	brz set_right

	ret


set_up:
	; cannot turn up while moving down
	mov eax DIRECTION
	mov ebx 1
	cmp eax ebx
	brz key_done

	mov eax 0
	mov NEXT_DIRECTION eax
	ret


set_down:
	; cannot turn down while moving up
	mov eax DIRECTION
	mov ebx 0
	cmp eax ebx
	brz key_done

	mov eax 1
	mov NEXT_DIRECTION eax
	ret


set_left:
	; cannot turn left while moving right
	mov eax DIRECTION
	mov ebx 3
	cmp eax ebx
	brz key_done

	mov eax 2
	mov NEXT_DIRECTION eax
	ret


set_right:
	; cannot turn right while moving left
	mov eax DIRECTION
	mov ebx 2
	cmp eax ebx
	brz key_done

	mov eax 3
	mov NEXT_DIRECTION eax


key_done:
	ret


restart_game:
	call reset_game
	call render
	ret


move_snake:
	; get the current head position
	mov r6 SNAKE_HEAD

	mov eax [r6]
	mov NEW_X eax

	mov ecx r6
	mov ebx 1
	add ecx ebx

	mov eax [ecx]
	mov NEW_Y eax

	; apply the new direction
	mov eax NEXT_DIRECTION
	mov DIRECTION eax

	mov ebx 0
	cmp eax ebx
	brz move_up

	mov ebx 1
	cmp eax ebx
	brz move_down

	mov ebx 2
	cmp eax ebx
	brz move_left

	bra move_right


move_up:
	mov eax NEW_Y
	mov ebx 1
	sub eax ebx
	mov NEW_Y eax

	bra check_position


move_down:
	mov eax NEW_Y
	mov ebx 1
	add eax ebx
	mov NEW_Y eax

	bra check_position


move_left:
	mov eax NEW_X
	mov ebx 1
	sub eax ebx
	mov NEW_X eax

	bra check_position


move_right:
	mov eax NEW_X
	mov ebx 1
	add eax ebx
	mov NEW_X eax


check_position:
	; check the left and right edges
	mov eax NEW_X
	mov ebx 0
	cmp eax ebx
	brn game_over

	mov ebx 80
	cmp eax ebx
	brn check_y

	bra game_over


check_y:
	; row 0 is reserved for the score
	mov eax NEW_Y
	mov ebx 1
	cmp eax ebx
	brn game_over

	mov ebx 25
	cmp eax ebx
	brn check_apple

	bra game_over


check_apple:
	mov eax 0
	mov EATING eax

	mov eax NEW_X
	mov ebx APPLE_X
	cmp eax ebx
	brz check_apple_y

	bra check_body


check_apple_y:
	mov eax NEW_Y
	mov ebx APPLE_Y
	cmp eax ebx
	brz eat_apple

	bra check_body


eat_apple:
	mov eax 1
	mov EATING eax


check_body:
	call hits_snake

	mov ebx 0
	cmp eax ebx
	brz add_head

	bra game_over


add_head:
	; move the head pointer to the next pair
	mov eax SNAKE_HEAD
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz head_wrap

	bra head_ready


head_wrap:
	mov eax SNAKE_START


head_ready:
	mov SNAKE_HEAD eax
	mov r6 eax

	; write the new head position
	mov ebx NEW_X
	mov [r6] ebx

	mov ecx r6
	mov eax 1
	add ecx eax

	mov ebx NEW_Y
	mov [ecx] ebx

	; leave the tail in place when eating
	mov eax EATING
	mov ebx 0
	cmp eax ebx
	brz remove_tail

	; grow the snake
	mov eax SNAKE_LENGTH
	mov ebx 1
	add eax ebx
	mov SNAKE_LENGTH eax

	; increase the score
	mov eax SCORE
	add eax ebx
	mov SCORE eax

	; check if the snake filled the board
	mov eax SNAKE_LENGTH
	mov ebx 1920
	cmp eax ebx
	brz game_won

	call place_apple
	ret


remove_tail:
	; move the tail pointer to the next pair
	mov eax SNAKE_TAIL
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz tail_wrap

	bra tail_ready


tail_wrap:
	mov eax SNAKE_START


tail_ready:
	mov SNAKE_TAIL eax
	ret


game_over:
	mov eax 1
	mov GAME_STATE eax
	ret


game_won:
	mov eax 2
	mov GAME_STATE eax
	ret


; eax = 1 if the new head hits the snake, otherwise 0
hits_snake:
	mov r6 SNAKE_TAIL
	mov r7 SNAKE_HEAD

	mov eax EATING
	mov ebx 0
	cmp eax ebx
	brz skip_old_tail

	bra body_cell


skip_old_tail:
	; the old tail moves away unless eating
	mov eax r6
	mov ebx r7
	cmp eax ebx
	brz body_free

	mov eax r6
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz body_start_wrap

	bra body_start_ready


body_start_wrap:
	mov eax SNAKE_START


body_start_ready:
	mov r6 eax


body_cell:
	; compare segment x
	mov eax [r6]
	mov ebx NEW_X
	cmp eax ebx
	brz body_check_y

	bra body_next


body_check_y:
	; compare segment y
	mov ecx r6
	mov ebx 1
	add ecx ebx

	mov eax [ecx]
	mov ebx NEW_Y
	cmp eax ebx
	brz body_hit


body_next:
	; stop after checking the head
	mov eax r6
	mov ebx r7
	cmp eax ebx
	brz body_free

	mov eax r6
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz body_wrap

	bra body_ready


body_wrap:
	mov eax SNAKE_START


body_ready:
	mov r6 eax
	bra body_cell


body_hit:
	mov eax 1
	ret


body_free:
	mov eax 0
	ret


random_number:
	mov eax RANDOM_NUMBER
	mov ebx 25173
	mul eax ebx

	mov ebx 13849
	add eax ebx

	mov RANDOM_NUMBER eax
	ret


place_apple:
	; get a random cell from 0 to 1919
	call random_number

	mov r8 eax
	mov ebx 1920
	udiv eax ebx
	umult eax ebx

	mov ebx r8
	sub ebx eax
	mov r8 ebx

	; y = cell / 80 + 1
	mov eax ebx
	mov ebx 80
	udiv eax ebx

	mov ecx eax
	mov edx 1
	add ecx edx
	mov APPLE_Y ecx

	; x = cell - (cell / 80) * 80
	umult eax ebx
	mov ebx r8
	sub ebx eax
	mov APPLE_X ebx

	; try again if the apple is inside the snake
	call apple_on_snake

	mov ebx 0
	cmp eax ebx
	brz apple_placed

	bra place_apple


apple_placed:
	ret


apple_on_snake:
	mov r6 SNAKE_TAIL
	mov r7 SNAKE_HEAD


apple_cell:
	mov eax [r6]
	mov ebx APPLE_X
	cmp eax ebx
	brz apple_check_y

	bra apple_next


apple_check_y:
	mov ecx r6
	mov ebx 1
	add ecx ebx

	mov eax [ecx]
	mov ebx APPLE_Y
	cmp eax ebx
	brz apple_occupied


apple_next:
	mov eax r6
	mov ebx r7
	cmp eax ebx
	brz apple_free

	mov eax r6
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz apple_wrap

	bra apple_ready


apple_wrap:
	mov eax SNAKE_START


apple_ready:
	mov r6 eax
	bra apple_cell


apple_occupied:
	mov eax 1
	ret


apple_free:
	mov eax 0
	ret


; draw snake and clear screen
; draw into the back buffer before copying to the screen
render:
	call screen_clear
	call draw_snake
	call draw_apple
	call draw_hud
	call screen_present
	ret


; clear screen background to blue
screen_clear:
	mov eax FRAME_END
	mov ebx 0x1020
	mov ecx FRAME_START
	mov edx 1


screen_clear_loop:
	mov [ecx] ebx
	add ecx edx

	cmp ecx eax
	brn screen_clear_loop

	ret


draw_snake:
	; draw snake on the screen
	; a segment of the snake is 2 words in size (x, y)
	; snake is white
	mov r6 SNAKE_TAIL
	mov r7 SNAKE_HEAD


draw_snake_loop:
	; get segment x
	mov eax [r6]

	; get segment y
	mov ecx r6
	mov ebx 1
	add ecx ebx
	mov ebx [ecx]

	; draw the segment
	mov ecx 1
	mov edx [draw_snake_next]
	bra pixel


draw_snake_next:
	mov eax r6
	mov ebx r7
	cmp eax ebx
	brz snake_drawn

	mov eax r6
	mov ebx 2
	add eax ebx

	mov ebx SNAKE_END
	cmp eax ebx
	brz draw_snake_wrap

	bra draw_snake_ready


draw_snake_wrap:
	mov eax SNAKE_START


draw_snake_ready:
	mov r6 eax
	bra draw_snake_loop


snake_drawn:
	ret


draw_apple:
	; do not draw an apple after winning
	mov eax GAME_STATE
	mov ebx 2
	cmp eax ebx
	brz apple_drawn

	mov eax APPLE_X
	mov ebx APPLE_Y
	mov ecx 2
	mov edx [apple_drawn]
	bra pixel


apple_drawn:
	ret


; eax = x (0–79), ebx = y (0–24)
; ecx = color: 0 blue, 1 white, 2 red
; edx = return address
; colors one text cell in the back buffer
pixel:
	mov r5 80
	mul ebx r5
	add eax ebx

	mov ebx FRAME_START
	add eax ebx

	mov ebx 0
	cmp ecx ebx
	brz pixel_blue

	mov ebx 2
	cmp ecx ebx
	brz pixel_red

	mov ebx 0xF020
	bra pixel_write


pixel_blue:
	mov ebx 0x1020
	bra pixel_write


pixel_red:
	mov ebx 0x4020


pixel_write:
	mov [eax] ebx
	bra edx


; copy the completed back buffer to the screen
screen_present:
	mov r6 FRAME_START
	mov r7 63534


screen_present_loop:
	mov eax [r6]
	mov [r7] eax

	mov ebx r6
	mov ecx 1
	add ebx ecx
	mov r6 ebx

	mov eax r7
	add eax ecx
	mov r7 eax

	mov eax FRAME_END
	cmp ebx eax
	brn screen_present_loop

	ret


draw_hud:
	; draw the score and controls
	mov r6 [HUD_TEXT_00]
	mov r7 FRAME_START
	call draw_text
	call draw_score

	; draw the current game state
	mov eax FRAME_START
	mov ebx 50
	add eax ebx
	mov r7 eax

	mov eax GAME_STATE
	mov ebx 0
	cmp eax ebx
	brz draw_playing

	mov ebx 1
	cmp eax ebx
	brz draw_game_over

	mov r6 [WIN_TEXT_00]
	bra draw_status


draw_playing:
	mov r6 [PLAYING_TEXT_00]
	bra draw_status


draw_game_over:
	mov r6 [GAME_OVER_TEXT_00]


draw_status:
	call draw_text
	ret


; r6 = text address, r7 = back buffer address
draw_text:
	mov eax [r6]
	mov ebx 0
	cmp eax ebx
	brz text_drawn

	mov [r7] eax

	mov eax r6
	mov ebx 1
	add eax ebx
	mov r6 eax

	mov eax r7
	add eax ebx
	mov r7 eax

	bra draw_text


text_drawn:
	ret


draw_score:
	mov r7 SCORE

	; address of the last score digit
	mov eax FRAME_START
	mov ebx 17
	add eax ebx
	mov r6 eax

	mov r8 4


draw_score_digit:
	; divide score by 10
	mov eax r7
	mov ebx 10
	udiv eax ebx
	mov r9 eax

	; get the remainder
	umult eax ebx
	mov ebx r7
	sub ebx eax

	; convert the digit to a text cell
	mov eax 0x1F30
	add ebx eax
	mov [r6] ebx

	mov r7 r9

	; move left to the next digit
	mov ecx r6
	mov eax 1
	sub ecx eax
	mov r6 ecx

	mov ecx r8
	sub ecx eax
	mov r8 ecx

	mov eax 0
	cmp ecx eax
	brz score_drawn

	bra draw_score_digit


score_drawn:
	ret


; INTERRUPT HANDLERS
keyboard_interupt:
	; handle keyboard interrupt
	mov last_key_pressed eax

	mov ebx 1
	mov keyHandled ebx

	iret


timer_interrupt:
	; increment the timer counter
	mov eax ms_passed
	mov ebx 1
	add eax ebx
	mov ms_passed eax

	; set random number generator
	call random_number

	iret

mouse_interrupt:
	iret

error_interrupt:
	; stop interrupts and the timer
	setirr 0
	mov eax 0
	mov 65535 eax

	; fill the visible screen with bright red
	mov eax 65534
	mov ebx 0xC020
	mov ecx 63534
	mov edx 1

error_screen_loop:
	mov [ecx] ebx
	add ecx edx

	cmp ecx eax
	brn error_screen_loop

	hlt