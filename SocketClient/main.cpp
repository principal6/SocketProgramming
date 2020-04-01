#include "CSocketClient.h"

int main()
{
	static constexpr int KInputBufferSize{ 2000 };
	char Input[KInputBufferSize]{};

	CSocketClient Client{ 27000 };

	assert(Client.Initialize());

	std::cout << "Enter server name (ip address): ";
	std::cin >> Input;
	assert(Client.Connect(Input));
	
	while (true)
	{
		memset(Input, 0, KInputBufferSize);
		std::cin >> Input;

		Client.Send(Input, (int)strlen(Input));
		if (Client.Receive() == false) break;
	}
	Client.ShutDownSending();

	std::cin.get();

	return 0;
}