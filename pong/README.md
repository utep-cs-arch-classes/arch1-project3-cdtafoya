#Pong
## Description
A simple game of pong for two players. This game incorporates many of the methods and structs for use with the LCD screen on the MSP430 given to us by Dr. Freudenthal. The game also uses state machines in order to end the game and make noises as the ball hits the walls and paddles.

## Running the Program
Makefile includes instructions to compile pong.c and all the necessary libraries for its use. Before running in this directory, be sure to type:

make

in its parent directory in order to set up all the files and libraries for use with the program. Once this has been done, inside the pong/ directory, please type:

make load

in order to load the program to the MSP430.

##Features
Pong is a simple game of ping pong in which the players must try to hit the wall behind their opponent's paddle. In this version, there is a Blue Player (Player 1) and Red Player (Player 2).

First player to 9 points wins.

The ball can be given effect when hit. This means it will go in the direction that the paddle is moving when it hits it.

Found in the code, is an option to make the paddles become smaller whenever a player scores to make it more interesting.

##Referenes
Dr. Freudenthal provided much of the code needed for this program, including the utiities needed to draw shapes on the LCD screen.

I used the provided shape-motion-demo.c as a base to write my program.