#!/usr/bin/python
import os
def run(sok):
	
	
	pid = os.fork()
	if not pid:
		os.execvp("python", ("python", "-cfrom Onboard.settings import Settings\ns = Settings(False)"))
	


