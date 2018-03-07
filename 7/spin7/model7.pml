ltl spec  {
	[] (((state == 0) && button) -> <> (state == 1)) &&

	[] (((state == 1) && button) -> <> (state == 2)) &&
	[] (((state == 1) && timeout) -> <> (state == 1)) &&

	[] (((state == 2) && button) -> <> (state == 3)) &&
	[] (((state == 2) && timeout) -> <> (state == 1)) &&

	[] (((state == 3) && button) -> <> (state == 4)) &&
	[] (((state == 3) && timeout) -> <> (state == 1)) &&

	[] (((state == 4) && button) -> <> (state == 5)) &&
	[] (((state == 4) && intruso) -> <> (state == 4)) &&
	
	[] (((state == 5) && button) -> <> (state == 6)) &&
	[] (((state == 5) && timeout) -> <> (state == 4)) &&

	[] (((state == 6) && button) -> <> (state == 7)) &&
	[] (((state == 6) && timeout) -> <> (state == 4)) &&

	[] (((state == 7) && button) -> <> (state == 0)) &&
	[] (((state == 7) && timeout) -> <> (state == 4))
}

#define timeout true

bit button;
bit intruso;
byte state;

active proctype alarma_codigo () {

	state = 0;
	do
	:: (state == 0) -> atomic {
		if
		:: button -> state = 1; button = 0
		fi
	}
	:: (state == 1) -> atomic {
		if
		:: button -> state = 2; button = 0
		:: timeout -> state = 0;
		fi
	}
	:: (state == 2) -> atomic {
		if
		:: button -> state = 3; button = 0
		:: timeout -> state = 0;
		fi
	}
	:: (state == 3) -> atomic {
		if
		:: button -> state = 4; button = 0
		:: timeout -> state = 0;
		fi
	}
	:: (state == 4) -> atomic {
		if
		:: button -> state = 5; button = 0
		:: intruso -> intruso = 0; state = 0;
		fi
	}
	:: (state == 5) -> atomic {
		if
		:: button -> state = 6; button = 0
		:: timeout -> state = 4;
		fi
	}
	:: (state == 6) -> atomic {
		if
		:: button -> state = 7; button = 0
		:: timeout -> state = 4;
		fi
	}
	:: (state == 7) -> atomic {
		if
		:: button -> state = 0; button = 0
		:: timeout -> state = 4;
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
