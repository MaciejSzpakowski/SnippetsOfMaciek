#include <iostream>

//combinations
void subsets(std::string front, std::string back)
{
	if (back.length() == 0)
	{
		printf("\"%s\"\n", front.c_str());
		return;
	}
	subsets(front + back[0], back.substr(1));
	subsets(front, back.substr(1));
}

void substrings(std::string str)
{
	for (int i = 0; i < str.length(); i++)
		for (int j = i; j < str.length(); j++)
			printf("\"%s\"\n", (str.substr(i, j - i + 1)).c_str());
}

void permutations(std::string front, std::string back)
{
	if (back.length() == 0)
	{
		printf("\"%s\"\n", front.c_str());
		return;
	}
	for (int i = 0; i < back.length();i++)
		permutations(front + back[i], back.substr(0,i) + back.substr(i+1));
}

int main()
{
	std::string str = "12345";

	printf("\nSubsets\n\n");
	subsets("", str);
	printf("\nSubstrings\n\n");
	substrings(str);
	printf("\nPermutations\n\n");
	permutations("", str);

	return 0;
}
