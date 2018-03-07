ltl spec  {
	[] (((state == 0) && code_ok) -> <> (state == 1)) &&
	[] (((state == 0) && code_no_ok) -> <> (state == 0)) &&
	[] (((state == 1) && code_ok) -> <> (state == 0)) &&
	[] (((state == 1) && code_no_ok) -> <> (state == 1)) &&
	[] (((state == 1) && timeout) -> <> (state == 1)) &&
	[] (((state == 1) && intruso) -> <> (state == 1))
}

#define timeout true

bit button;
bit intruso;
bit code_ok;
bit code_no_ok;
byte state;

active proctype maquinas_estados_concurrentes () {

	state = 0;
	do
	:: (state == 0) -> atomic {
		if
		:: code_ok -> state = 1; code_ok = 0;
		:: code_no_ok -> state = 0; code_no_ok = 0;
		fi
	}
	:: (state == 1) -> atomic {
		if
		:: code_ok -> state = 0; code_ok = 0;
		:: code_no_ok -> state = 1; code_no_ok = 0;
		:: intruso -> intruso = 0; state = 1;
		:: timeout -> state = 1;
		fi
	}
	od
}

active proctype entorno () {
	do
	:: code_ok = 1
	:: code_ok = 0
	:: code_no_ok = 1
	:: code_no_ok = 0
	:: intruso = 1
	:: (!intruso) -> skip
	od
}
