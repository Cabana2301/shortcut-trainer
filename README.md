# shortcut-trainer
A win32 utility to practice keyboard shortcuts

## Notes
Put the shortcuts that you want to practice in input.txt  
You can see the list of key names in virtual-keys.txt  
Make sure you have both files in your working directory  
(usually where you have the exe file)  
If you're running inside VisualStudio, don't worry about it.

## Why this exists
I recently got a new mechanical ergonomic keyboard and I wrote 
this to get good at using it.  
Partly because I wanted what this program does and partly because 
I wanted a programming excercise for the keyboard that isn't my day-job.

### This is unpolished work, thank you for noticing
This is my first time publishing work on GitHub.  
This is also my first time working with Win32 directly,
I wouldn't normally do this and would otherwise opt
for the console (in Linux, most likely).  
This time I wanted something that runs natively
on my own PC and I noticed that the Terminal in Windows11
intercepts many keyboard shortcuts so they never reach my
application.

It was much more difficult for me to figure out how to do
this than I thought it will be but I'm pretty happy with the result.  
Specifically, figuring out that I should subclass the EDIT child to get keyboard events and getting it to work was a pain.  
I don't presume to say that I did it "properly", but I found a way that works.

### Due credits
https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Controls/use-a-multiline-edit-control.md  
https://learn.microsoft.com/en-us/windows/win32/controls/subclass-a-combo-box 
https://learn.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-