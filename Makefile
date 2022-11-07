compiler = build/ns-allinone-3.37/ns-3.37/ns3
buildpath = build/ns-allinone-3.37/ns-3.37/scratch
src = main.cc
bin = scratch/main

compile:
	@cp $(src) $(buildpath)
	@$(compiler) --quiet

install:
	@mkdir build
	@wget https://www.nsnam.org/releases/ns-allinone-3.37.tar.bz2 -P build
	@tar -xf build/ns-allinone-3.37.tar.bz2 -C build

	@./build/ns-allinone-3.37/ns-3.37/ns3 configure
	@./build/ns-allinone-3.37/ns-3.37/ns3 build
	@cd build/ns-allinone-3.37/netanim-3.108 && qmake NetAnim.Pro && make


run: compile
	@$(compiler) run $(bin) --quiet

debug: compile
	@$(compiler) run --gdb $(bin) --quiet

netanim:
	@screen -dmS netanim ./build/ns-allinone-3.37/netanim-3.108/NetAnim.pro

clean:
	@rm -rf build