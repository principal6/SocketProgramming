#include "CSocketServer.h"

int main()
{
	CSocketServer Server{ 27000 };

	assert(Server.Initialize());
	
	Server.Listen();

	while (true)
	{
		if (Server.Receive() == false) break;

		if (GetAsyncKeyState(VK_ESCAPE) < 0) break;
	}
	
	Server.ShutDownSending();

	std::cin.get();

	return 0;
}