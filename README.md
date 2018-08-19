This program trains a Monte Carlo tree search/neural network model to self play the crafting
minigame in Final Fantasy XIV (an online video game by SquareEnix).  I wrote this to learn the
AlphaGo Zero paper (https://deepmind.com/documents/119/agz_unformatted_nature.pdf): the program is
basically a much simplified AlphaGo Zero on a different game.

The program doesn't interact with the actual video game, just performs self play using a game engine
that simulates the crafting minigame.  Using the original video game for self play & neural network
training is probably possible but will be impractically slow.  Unlike Go, the crafting minigame is a
one player game with the goal of maximizing a gameplay score determined by the sequence of actions /
moves performed.  It also contains some random chances as each in-game action is associated with a
successful rate with different outcomes associated with success and failure.

For an introduction to the crafting minigame in FFXIV, see for example
https://ffxiv.consolegameswiki.com/wiki/Crafting.

This program depends only on the standard C++ library.

Gameplay Performance

The program achieves quite good gameplay performance based on my knowledge of the crafting minigame.
However, a few factors make it hard to accurately evaluate the exact performance: the crafting
minigame in FFXIV is a one player game, so I can't play against it myself to determine how strong it
is.  The only way to evaluate its performance is by examining the target score, this is also
somewhat difficult given the resource I have.  The minigame involves random changes which makes the
result score noisy.  Roughly 1000 gameplay is required to determine its average performance, while
it's very easy to run this program 1000 times and collect the statistics, it takes too much effort
to collect statistics from actual human gameplay.

That being said, my rough estimation is that it can achieve 90% performance of a human player after
around 10000 game plays, although this 90% figure is somewhat arbitrary and based on "rule of thumb"
examination of strategies employed in self play.  On my machine the gameplay AI converges to a very
recognizable strategy pattern after around 4-5 days of training.  The gameplay decisions made by the
trained AI display some remarkable learned knowledge.  The simplest example is that standard
gameplay strategies to craft high quality items all involve accumulating an "inner quiet" stack and
consumes the stack towards the end, the same pattern is also observed in the sample gameplay of this
program.

Note: the gameplay engine in this program reflects the state of the crafting minigame as of February
2018.  Since the original game receives constant update, it's likely that the version simulated by
this program deviates from the up-to-date crafting game in FFXIV in some way as time goes on (e.g.,
new actions / recipes may be introduced).  But the basic gameplay concept probably will not change
much.

Usage

You need a C++ compiler that supports C++ 14.  I tested with gcc 8.2.0 and clang 6.0.1.  You don't
need any other software library.

Example,

$ make
$ ./ndebug | tee log
