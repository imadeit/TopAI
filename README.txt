This project incluse some ai and machine learing programs. A partial of the is my homework in JAIST.

Prepare for building:
    - The auto configuration tool is base on Windows PowerShell script.
	  If this is your first time using PowerShell, import a registry file
      ".\tool\powershell-enable.reg" to enable PowerShell script.	  
(1) Start up Windows PowerShell
(2) Enter direct "<workspace>\tool"
	- We suppose the <workspace> dir is trunk. If you download all reporsitory as your workspace, please uset "<workspace>\trunk\tool"
(3) Run ".\auto_config.ps1"
	.\auto_config.ps1 [-path_vld <VLD_PATH>] -path_boost <BOOST_PATH> -path_opencv <OPENCV_PATH>
	

Necessary libraries
		necessary ver.		tested ver.
vld		1.5 included		1.5 / 2.1
boost						1.46.1 / 1.59.0
opencv						2.3.1					
