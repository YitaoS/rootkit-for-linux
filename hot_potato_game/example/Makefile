TARGETS=player ringmaster

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp hot_potato.hpp
	g++ -g -o $@ $<

ringmaster: ringmaster.cpp hot_potato.hpp
	g++ -g -o $@ $<

