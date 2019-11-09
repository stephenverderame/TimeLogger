# TimeLogger
Simple application to record the time spent in each program.

Has two components, the UI and the background process applications.
`GettingRunningProcesses.exe` records all user applications with a visible window, and records which application is currently active.
It updates at roughly one minute intervals and saves data to a text file when the computer powers off.
It is reccommended to add it to your startup programs so that it automatically runs when your starts.


The UI application can be run to easily display the information. Three times are displayed for each application. A total running time, 
a resettable running time and an active time. The active time records how long the application has been the one currently being worked in
and the resettable running time is the total running time except you can reset this timer by clicking the `Reset Relative` button. The
application also displays a percentage and progress bar which represents the percent of the computer's total running time in which that application was open.
Double click the name of each application to view its data.
