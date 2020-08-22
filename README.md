# x16-edit
Text editor for Commander X16

X16 Edit is a simple text editor for the Commander X16 platform inspired by GNU Nano but lacking some of its more advanced functions such as soft line breaks.

The program is written in 65c02 assembly, and does not share any code with GNU Nano.

Text entered by the user is stored in banked RAM, and it will fit nearly 2 MB of actual text.

The text is, however, not stored in banked RAM as a continuous string. If memory usage was structured as a continuous string, it would in particular make 
inserting text at the top very slow, as the program would need to move/copy all subsequent text forward to make room for the text to be inserted.

To avoid that, each memory page contains a separate string. The memory pages/strings are linked to each other. The order in which they are linked is
not decided beforehand, and it may be changed during the operation of the program.

Memory page layout in banked RAM:

Offset Content
00     Previous memory page/bank number
01     Previous memory page/address most significant byte; 0 indicates start of file
02     Next memory page/bank number
03     Next memory page/address most significant byte; 0 indicates end of file
04     Length of text in this memory page (0-251 bytes)
05-ff  Text (251 bytes)

You may use the built in monitor to manually check how this works. The first memory page is at bank 1, address $a000.
