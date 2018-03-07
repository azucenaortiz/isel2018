ltl spec  {
	[] (((state == 0) && button) -> <> (state == 1)) &&
	[] (((state == 1) && button) -> <> (state == 0)) &&
	[] (((state == 1) && intruso) -> <> (state == 1))
}

bit button;
bit intruso;
byte state;

active proctype alarma_basica () {

	state = 0;
	do
	:: (state == 0) -> atomic {
		if
		:: button -> state = 1; button = 0
		fi
	}
	:: (state == 1) -> atomic {
		if
		:: button -> state = 0;
		:: intruso -> intruso = 0; state = 1;
		fi
	}
	od
}

active proctype entorno () {
	do
	:: button = 1
	:: (!button) -> skip
	:: intruso = 1
	:: (!intruso) -> skip
	od
}
