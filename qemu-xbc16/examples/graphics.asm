.16bc
; Text/input demo: raw QEMU scancodes are separate from printable CP437.
; Printable mapping is US keyboard; Shift and Caps Lock are handled.
.data
WORD keys
WORD mice
WORD rawkey
WORD leftshift
WORD rightshift
WORD capslock
WORD plain0 0
WORD plain1 0
WORD plain2 49
WORD plain3 50
WORD plain4 51
WORD plain5 52
WORD plain6 53
WORD plain7 54
WORD plain8 55
WORD plain9 56
WORD plain10 57
WORD plain11 48
WORD plain12 45
WORD plain13 61
WORD plain14 0
WORD plain15 0
WORD plain16 113
WORD plain17 119
WORD plain18 101
WORD plain19 114
WORD plain20 116
WORD plain21 121
WORD plain22 117
WORD plain23 105
WORD plain24 111
WORD plain25 112
WORD plain26 91
WORD plain27 93
WORD plain28 0
WORD plain29 0
WORD plain30 97
WORD plain31 115
WORD plain32 100
WORD plain33 102
WORD plain34 103
WORD plain35 104
WORD plain36 106
WORD plain37 107
WORD plain38 108
WORD plain39 59
WORD plain40 39
WORD plain41 96
WORD plain42 0
WORD plain43 92
WORD plain44 122
WORD plain45 120
WORD plain46 99
WORD plain47 118
WORD plain48 98
WORD plain49 110
WORD plain50 109
WORD plain51 44
WORD plain52 46
WORD plain53 47
WORD plain54 0
WORD plain55 0
WORD plain56 0
WORD plain57 32
WORD plain58 0
WORD plain59 0
WORD plain60 0
WORD plain61 0
WORD plain62 0
WORD plain63 0
WORD plain64 0
WORD plain65 0
WORD plain66 0
WORD plain67 0
WORD plain68 0
WORD plain69 0
WORD plain70 0
WORD plain71 0
WORD plain72 0
WORD plain73 0
WORD plain74 0
WORD plain75 0
WORD plain76 0
WORD plain77 0
WORD plain78 0
WORD plain79 0
WORD plain80 0
WORD plain81 0
WORD plain82 0
WORD plain83 0
WORD plain84 0
WORD plain85 0
WORD plain86 0
WORD plain87 0
WORD plain88 0
WORD plain89 0
WORD plain90 0
WORD plain91 0
WORD plain92 0
WORD plain93 0
WORD plain94 0
WORD plain95 0
WORD plain96 0
WORD plain97 0
WORD plain98 0
WORD plain99 0
WORD plain100 0
WORD plain101 0
WORD plain102 0
WORD plain103 0
WORD plain104 0
WORD plain105 0
WORD plain106 0
WORD plain107 0
WORD plain108 0
WORD plain109 0
WORD plain110 0
WORD plain111 0
WORD plain112 0
WORD plain113 0
WORD plain114 0
WORD plain115 0
WORD plain116 0
WORD plain117 0
WORD plain118 0
WORD plain119 0
WORD plain120 0
WORD plain121 0
WORD plain122 0
WORD plain123 0
WORD plain124 0
WORD plain125 0
WORD plain126 0
WORD plain127 0
WORD shift0 0
WORD shift1 0
WORD shift2 33
WORD shift3 64
WORD shift4 35
WORD shift5 36
WORD shift6 37
WORD shift7 94
WORD shift8 38
WORD shift9 42
WORD shift10 40
WORD shift11 41
WORD shift12 95
WORD shift13 43
WORD shift14 0
WORD shift15 0
WORD shift16 81
WORD shift17 87
WORD shift18 69
WORD shift19 82
WORD shift20 84
WORD shift21 89
WORD shift22 85
WORD shift23 73
WORD shift24 79
WORD shift25 80
WORD shift26 123
WORD shift27 125
WORD shift28 0
WORD shift29 0
WORD shift30 65
WORD shift31 83
WORD shift32 68
WORD shift33 70
WORD shift34 71
WORD shift35 72
WORD shift36 74
WORD shift37 75
WORD shift38 76
WORD shift39 58
WORD shift40 34
WORD shift41 126
WORD shift42 0
WORD shift43 124
WORD shift44 90
WORD shift45 88
WORD shift46 67
WORD shift47 86
WORD shift48 66
WORD shift49 78
WORD shift50 77
WORD shift51 60
WORD shift52 62
WORD shift53 63
WORD shift54 0
WORD shift55 0
WORD shift56 0
WORD shift57 32
WORD shift58 0
WORD shift59 0
WORD shift60 0
WORD shift61 0
WORD shift62 0
WORD shift63 0
WORD shift64 0
WORD shift65 0
WORD shift66 0
WORD shift67 0
WORD shift68 0
WORD shift69 0
WORD shift70 0
WORD shift71 0
WORD shift72 0
WORD shift73 0
WORD shift74 0
WORD shift75 0
WORD shift76 0
WORD shift77 0
WORD shift78 0
WORD shift79 0
WORD shift80 0
WORD shift81 0
WORD shift82 0
WORD shift83 0
WORD shift84 0
WORD shift85 0
WORD shift86 0
WORD shift87 0
WORD shift88 0
WORD shift89 0
WORD shift90 0
WORD shift91 0
WORD shift92 0
WORD shift93 0
WORD shift94 0
WORD shift95 0
WORD shift96 0
WORD shift97 0
WORD shift98 0
WORD shift99 0
WORD shift100 0
WORD shift101 0
WORD shift102 0
WORD shift103 0
WORD shift104 0
WORD shift105 0
WORD shift106 0
WORD shift107 0
WORD shift108 0
WORD shift109 0
WORD shift110 0
WORD shift111 0
WORD shift112 0
WORD shift113 0
WORD shift114 0
WORD shift115 0
WORD shift116 0
WORD shift117 0
WORD shift118 0
WORD shift119 0
WORD shift120 0
WORD shift121 0
WORD shift122 0
WORD shift123 0
WORD shift124 0
WORD shift125 0
WORD shift126 0
WORD shift127 0
.code
mov eax 0x1720
mov ecx 63534
mov edx 65534
fill: mov [ecx] eax
mov ebx 1
add ecx ebx
cmp ecx edx
brn fill
mov eax 7985
mov 63534 eax
mov eax 7990
mov 63535 eax
mov eax 8002
mov 63536 eax
mov eax 8003
mov 63537 eax
mov eax 7968
mov 63538 eax
mov eax 7981
mov 63539 eax
mov eax 7968
mov 63540 eax
mov eax 8022
mov 63541 eax
mov eax 8007
mov 63542 eax
mov eax 8001
mov 63543 eax
mov eax 7968
mov 63544 eax
mov eax 8020
mov 63545 eax
mov eax 8005
mov 63546 eax
mov eax 8024
mov 63547 eax
mov eax 8020
mov 63548 eax
mov eax 7968
mov 63549 eax
mov eax 7983
mov 63550 eax
mov eax 7968
mov 63551 eax
mov eax 8009
mov 63552 eax
mov eax 8014
mov 63553 eax
mov eax 8016
mov 63554 eax
mov eax 8021
mov 63555 eax
mov eax 8020
mov 63556 eax
mov eax 7968
mov 63557 eax
mov eax 8020
mov 63558 eax
mov eax 8005
mov 63559 eax
mov eax 8019
mov 63560 eax
mov eax 8020
mov 63561 eax
mov eax 7755
mov 63694 eax
mov eax 7781
mov 63695 eax
mov eax 7801
mov 63696 eax
mov eax 7778
mov 63697 eax
mov eax 7791
mov 63698 eax
mov eax 7777
mov 63699 eax
mov eax 7794
mov 63700 eax
mov eax 7780
mov 63701 eax
mov eax 7712
mov 63702 eax
mov eax 7781
mov 63703 eax
mov eax 7798
mov 63704 eax
mov eax 7781
mov 63705 eax
mov eax 7790
mov 63706 eax
mov eax 7796
mov 63707 eax
mov eax 7795
mov 63708 eax
mov eax 7712
mov 63709 eax
mov eax 7720
mov 63710 eax
mov eax 7784
mov 63711 eax
mov eax 7781
mov 63712 eax
mov eax 7800
mov 63713 eax
mov eax 7712
mov 63714 eax
mov eax 7790
mov 63715 eax
mov eax 7785
mov 63716 eax
mov eax 7778
mov 63717 eax
mov eax 7778
mov 63718 eax
mov eax 7788
mov 63719 eax
mov eax 7781
mov 63720 eax
mov eax 7721
mov 63721 eax
mov eax 7738
mov 63722 eax
mov eax 7712
mov 63723 eax
mov eax 7728
mov 63724 eax
mov eax 8012
mov 63774 eax
mov eax 8033
mov 63775 eax
mov eax 8051
mov 63776 eax
mov eax 8052
mov 63777 eax
mov eax 7968
mov 63778 eax
mov eax 8035
mov 63779 eax
mov eax 8040
mov 63780 eax
mov eax 8033
mov 63781 eax
mov eax 8050
mov 63782 eax
mov eax 8033
mov 63783 eax
mov eax 8035
mov 63784 eax
mov eax 8052
mov 63785 eax
mov eax 8037
mov 63786 eax
mov eax 8050
mov 63787 eax
mov eax 7968
mov 63788 eax
mov eax 7976
mov 63789 eax
mov eax 8021
mov 63790 eax
mov eax 8019
mov 63791 eax
mov eax 7968
mov 63792 eax
mov eax 8045
mov 63793 eax
mov eax 8033
mov 63794 eax
mov eax 8048
mov 63795 eax
mov eax 7977
mov 63796 eax
mov eax 7994
mov 63797 eax
mov eax 8018
mov 63854 eax
mov eax 8033
mov 63855 eax
mov eax 8055
mov 63856 eax
mov eax 7968
mov 63857 eax
mov eax 8017
mov 63858 eax
mov eax 8005
mov 63859 eax
mov eax 8013
mov 63860 eax
mov eax 8021
mov 63861 eax
mov eax 7968
mov 63862 eax
mov eax 8043
mov 63863 eax
mov eax 8037
mov 63864 eax
mov eax 8057
mov 63865 eax
mov eax 8035
mov 63866 eax
mov eax 8047
mov 63867 eax
mov eax 8036
mov 63868 eax
mov eax 8037
mov 63869 eax
mov eax 7994
mov 63870 eax
mov eax 7968
mov 63871 eax
mov eax 7984
mov 63872 eax
mov eax 8056
mov 63873 eax
mov eax 7984
mov 63874 eax
mov eax 7984
mov 63875 eax
mov eax 7984
mov 63876 eax
mov eax 7984
mov 63877 eax
mov eax 7757
mov 63934 eax
mov eax 7791
mov 63935 eax
mov eax 7797
mov 63936 eax
mov eax 7795
mov 63937 eax
mov eax 7781
mov 63938 eax
mov eax 7712
mov 63939 eax
mov eax 7781
mov 63940 eax
mov eax 7798
mov 63941 eax
mov eax 7781
mov 63942 eax
mov eax 7790
mov 63943 eax
mov eax 7796
mov 63944 eax
mov eax 7795
mov 63945 eax
mov eax 7712
mov 63946 eax
mov eax 7720
mov 63947 eax
mov eax 7784
mov 63948 eax
mov eax 7781
mov 63949 eax
mov eax 7800
mov 63950 eax
mov eax 7712
mov 63951 eax
mov eax 7790
mov 63952 eax
mov eax 7785
mov 63953 eax
mov eax 7778
mov 63954 eax
mov eax 7778
mov 63955 eax
mov eax 7788
mov 63956 eax
mov eax 7781
mov 63957 eax
mov eax 7721
mov 63958 eax
mov eax 7738
mov 63959 eax
mov eax 7712
mov 63960 eax
mov eax 7728
mov 63961 eax
mov eax 8002
mov 64014 eax
mov eax 8053
mov 64015 eax
mov eax 8052
mov 64016 eax
mov eax 8052
mov 64017 eax
mov eax 8047
mov 64018 eax
mov eax 8046
mov 64019 eax
mov eax 8051
mov 64020 eax
mov eax 7968
mov 64021 eax
mov eax 7976
mov 64022 eax
mov eax 7984
mov 64023 eax
mov eax 7981
mov 64024 eax
mov eax 7991
mov 64025 eax
mov eax 7977
mov 64026 eax
mov eax 7994
mov 64027 eax
mov eax 7968
mov 64028 eax
mov eax 7984
mov 64029 eax
mov eax 8003
mov 64174 eax
mov eax 8044
mov 64175 eax
mov eax 8041
mov 64176 eax
mov eax 8035
mov 64177 eax
mov eax 8043
mov 64178 eax
mov eax 7968
mov 64179 eax
mov eax 8052
mov 64180 eax
mov eax 8040
mov 64181 eax
mov eax 8041
mov 64182 eax
mov eax 8051
mov 64183 eax
mov eax 7968
mov 64184 eax
mov eax 8055
mov 64185 eax
mov eax 8041
mov 64186 eax
mov eax 8046
mov 64187 eax
mov eax 8036
mov 64188 eax
mov eax 8047
mov 64189 eax
mov eax 8055
mov 64190 eax
mov eax 7968
mov 64191 eax
mov eax 8033
mov 64192 eax
mov eax 8046
mov 64193 eax
mov eax 8036
mov 64194 eax
mov eax 7968
mov 64195 eax
mov eax 8052
mov 64196 eax
mov eax 8057
mov 64197 eax
mov eax 8048
mov 64198 eax
mov eax 8037
mov 64199 eax
mov eax 7982
mov 64200 eax
mov eax 7968
mov 64201 eax
mov eax 8007
mov 64202 eax
mov eax 8050
mov 64203 eax
mov eax 8033
mov 64204 eax
mov eax 8034
mov 64205 eax
mov eax 7968
mov 64206 eax
mov eax 8045
mov 64207 eax
mov eax 8047
mov 64208 eax
mov eax 8053
mov 64209 eax
mov eax 8051
mov 64210 eax
mov eax 8037
mov 64211 eax
mov eax 7968
mov 64212 eax
mov eax 8038
mov 64213 eax
mov eax 8047
mov 64214 eax
mov eax 8050
mov 64215 eax
mov eax 7968
mov 64216 eax
mov eax 8050
mov 64217 eax
mov eax 8037
mov 64218 eax
mov eax 8044
mov 64219 eax
mov eax 8033
mov 64220 eax
mov eax 8052
mov 64221 eax
mov eax 8041
mov 64222 eax
mov eax 8054
mov 64223 eax
mov eax 8037
mov 64224 eax
mov eax 7968
mov 64225 eax
mov eax 8045
mov 64226 eax
mov eax 8047
mov 64227 eax
mov eax 8052
mov 64228 eax
mov eax 8041
mov 64229 eax
mov eax 8047
mov 64230 eax
mov eax 8046
mov 64231 eax
mov eax 7982
mov 64232 eax
mov eax 8003
mov 64254 eax
mov eax 8052
mov 64255 eax
mov eax 8050
mov 64256 eax
mov eax 8044
mov 64257 eax
mov eax 7979
mov 64258 eax
mov eax 8001
mov 64259 eax
mov eax 8044
mov 64260 eax
mov eax 8052
mov 64261 eax
mov eax 7979
mov 64262 eax
mov eax 8007
mov 64263 eax
mov eax 7968
mov 64264 eax
mov eax 8050
mov 64265 eax
mov eax 8037
mov 64266 eax
mov eax 8044
mov 64267 eax
mov eax 8037
mov 64268 eax
mov eax 8033
mov 64269 eax
mov eax 8051
mov 64270 eax
mov eax 8037
mov 64271 eax
mov eax 8051
mov 64272 eax
mov eax 7968
mov 64273 eax
mov eax 8019
mov 64274 eax
mov eax 8004
mov 64275 eax
mov eax 8012
mov 64276 eax
mov eax 7968
mov 64277 eax
mov eax 8045
mov 64278 eax
mov eax 8047
mov 64279 eax
mov eax 8053
mov 64280 eax
mov eax 8051
mov 64281 eax
mov eax 8037
mov 64282 eax
mov eax 7968
mov 64283 eax
mov eax 8039
mov 64284 eax
mov eax 8050
mov 64285 eax
mov eax 8033
mov 64286 eax
mov eax 8034
mov 64287 eax
mov eax 7982
mov 64288 eax
mov eax 7968
mov 64289 eax
mov eax 8003
mov 64290 eax
mov eax 8044
mov 64291 eax
mov eax 8047
mov 64292 eax
mov eax 8051
mov 64293 eax
mov eax 8037
mov 64294 eax
mov eax 7968
mov 64295 eax
mov eax 8055
mov 64296 eax
mov eax 8041
mov 64297 eax
mov eax 8046
mov 64298 eax
mov eax 8036
mov 64299 eax
mov eax 8047
mov 64300 eax
mov eax 8055
mov 64301 eax
mov eax 7968
mov 64302 eax
mov eax 8052
mov 64303 eax
mov eax 8047
mov 64304 eax
mov eax 7968
mov 64305 eax
mov eax 8037
mov 64306 eax
mov eax 8056
mov 64307 eax
mov eax 8041
mov 64308 eax
mov eax 8052
mov 64309 eax
mov eax 7982
mov 64310 eax
mov eax keyboard
mov 63278 eax
mov eax mouse
mov 63279 eax
setirr 1
wait: bra wait
keyboard:
mov rawkey eax
mov eax rawkey
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
mov ebx 15
and eax ebx
mov ebx 10
cmp eax ebx
brn hex_digit0
mov ebx 7
add eax ebx
hex_digit0: mov ebx 0x1f30
add eax ebx
mov 63873 eax
mov eax rawkey
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
rshift eax
mov ebx 15
and eax ebx
mov ebx 10
cmp eax ebx
brn hex_digit1
mov ebx 7
add eax ebx
hex_digit1: mov ebx 0x1f30
add eax ebx
mov 63874 eax
mov eax rawkey
rshift eax
rshift eax
rshift eax
rshift eax
mov ebx 15
and eax ebx
mov ebx 10
cmp eax ebx
brn hex_digit2
mov ebx 7
add eax ebx
hex_digit2: mov ebx 0x1f30
add eax ebx
mov 63875 eax
mov eax rawkey
mov ebx 15
and eax ebx
mov ebx 10
cmp eax ebx
brn hex_digit3
mov ebx 7
add eax ebx
hex_digit3: mov ebx 0x1f30
add eax ebx
mov 63876 eax
mov edx rawkey
mov ebx 0x7fff
and edx ebx
mov ebx 42
cmp edx ebx
brz left_shift
mov ebx 54
cmp edx ebx
brz right_shift
mov ebx 58
cmp edx ebx
brz caps_key
mov eax rawkey
mov ebx 0x8000
and eax ebx
mov ebx 0
cmp eax ebx
brz printable
bra keyboard_count
left_shift: mov eax rawkey
mov ebx 0x8000
and eax ebx
mov ebx 0
cmp eax ebx
brz left_down
mov eax 0
mov leftshift eax
bra keyboard_count
left_down: mov eax 1
mov leftshift eax
bra keyboard_count
right_shift: mov eax rawkey
mov ebx 0x8000
and eax ebx
mov ebx 0
cmp eax ebx
brz right_down
mov eax 0
mov rightshift eax
bra keyboard_count
right_down: mov eax 1
mov rightshift eax
bra keyboard_count
caps_key: mov eax rawkey
mov ebx 0x8000
and eax ebx
mov ebx 0
cmp eax ebx
brz caps_down
bra keyboard_count
caps_down: mov eax capslock
mov ebx 1
xor eax ebx
mov capslock eax
bra keyboard_count
printable: mov ebx 128
cmp edx ebx
brn lookup
bra keyboard_count
lookup: mov ecx [plain0]
add ecx edx
mov eax [ecx]
mov ebx 0
cmp eax ebx
brz keyboard_count
mov ecx leftshift
mov ebx rightshift
or ecx ebx
mov ebx 97
cmp eax ebx
brn choose_map
mov ebx 123
cmp eax ebx
brn letter_map
bra choose_map
letter_map: mov ebx capslock
xor ecx ebx
choose_map: mov ebx 0
cmp ecx ebx
brz show_character
mov ecx [shift0]
add ecx edx
mov eax [ecx]
show_character: mov ebx 0x1f00
or eax ebx
mov 63799 eax
keyboard_count: mov eax keys
mov ebx 1
add eax ebx
mov keys eax
mov ecx 63724
bra counter
mouse: mov ebx 0x1f30
or eax ebx
mov 64029 eax
mov eax mice
mov ebx 1
add eax ebx
mov mice eax
mov ecx 63961
counter: mov ebx 15
and eax ebx
mov ebx 10
cmp eax ebx
brn digit
mov ebx 7
add eax ebx
digit: mov ebx 0x1e30
add eax ebx
mov [ecx] eax
; Discard the 15-word IRQ snapshot; the demo has no live register state.
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
pop eax
setirr 1
bra wait
