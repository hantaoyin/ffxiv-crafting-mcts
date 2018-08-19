default: ndebug

debug: debug_logging.h ffxiv-crafting-mcts.C
	g++ -std=c++14 -O3 -Wall -Wextra ffxiv-crafting-mcts.C -o ffxiv-crafting-mcts

ndebug: debug_logging.h ffxiv-crafting-mcts.C
	g++ -std=c++14 -O3 -Wall -Wextra ffxiv-crafting-mcts.C -DNDEBUG -o ffxiv-crafting-mcts

clean:
	-rm -f ffxiv-crafting-mcts
