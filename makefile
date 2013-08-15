# Simple make file showing the commandline arguments to g++
# used to compile a simple example with kodo
# Note, that the paths to the libraries depend
# on your specific machine and will need to be updated.

# The include path to the kodo sources
kodo_dir ?= ~/raspKodo/kodo/src

# The include path to the sak sources
sak_dir ?= ~/raspKodo/kodo/bundle_dependencies/sak-1bdcea/master/src/

# The include path to the fifi sources
fifi_dir ?= ~/raspKodo/kodo/bundle_dependencies/fifi-8960fd/master/src/

# The include path to the boost sources
boost_dir ?= ~/raspKodo/kodo/bundle_dependencies/boost-11f274/master/

# Invoke the compiler
all:
	#g++ -std=c++0x  BroadcastSender.cpp PracticalSocket.cpp -o broadcastsender -I $(boost_dir) -I $(fifi_dir) -I $(kodo_dir) -I $(sak_dir)
	#g++ -std=c++0x  BroadcastReceiver.cpp PracticalSocket.cpp -o broadcastreceiver -I $(boost_dir) -I $(fifi_dir) -I $(kodo_dir) -I $(sak_dir)
	#g++ main.cpp -o test --std=c++0x -I $(boost_dir) -I $(fifi_dir) -I $(kodo_dir) -I $(sak_dir)
	$(CXX) -std=c++0x -D_GLIBCXX_USE_NANOSLEEP rasp.cpp PracticalSocket.cpp -o rasp -I $(boost_dir) -I $(fifi_dir) -I $(kodo_dir) -I $(sak_dir) -Wl,-Bstatic -lgflags -Wl,-Bdynamic
	
