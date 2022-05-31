#include "Network.h"

int main() {
	try {
		Network network;
		network.Update();
	}
	catch (Exception& ex)
	{
		cout << ex.what() << "\n";
	}
}