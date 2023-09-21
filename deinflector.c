#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

void itouform(wchar_t word[], int len)
{
    wchar_t iforms[] = L"いきしちにみりぎ";
    wchar_t uforms[] = L"うくすつぬむるぐ";

    wchar_t lastchr = word[len - 1];
    word[len - 1] = L'\0';
    int i = 0;
    while (iforms[i] != lastchr && i <= 7)
      i++;

    if (i <= 7)
      wprintf(L"%ls%lc\n", word, uforms[i]);

    wprintf(L"%ls%lcる\n", word, lastchr);
    exit(EXIT_SUCCESS);
}

void atouform(wchar_t word[], int len)
{
    wchar_t aforms[] = L"さかがばたまわらな";
    wchar_t uforms[] = L"すくぐぶつむうるぬ";

    wchar_t lastchr = word[len - 1];
    word[len - 1] = L'\0';
    int i = 0;
    while (aforms[i] != lastchr && i <= 9)
      i++;

    if (i <= 9)
      wprintf(L"%ls%lc\n", word, uforms[i]);

    wprintf(L"%ls%lcる\n", word, lastchr);
    return;
}

void te_form(wchar_t word[], int len)
{
    wchar_t lastchr = word[len - 1];
    word[len - 1] = L'\0';

    if (lastchr == L'し')
    {
	wprintf(L"%lsす\n", word);
    }
    else if (lastchr == L'い')
    {
	wprintf(L"%lsく\n", word);
    }
    else if (lastchr == L'ん')
    {
	wprintf(L"%lsむ\n", word);
	wprintf(L"%lsぶ\n", word);
	wprintf(L"%lsぬ\n", word);
    }
    else if (lastchr == L'っ')
    {
	if (word[len - 2] == L'行')
	{
	    wprintf(L"%lsく\n", word);
	}
	else
	{
	    wprintf(L"%lsる\n", word);
	    wprintf(L"%lsう\n", word);
	    wprintf(L"%lsつ\n", word);
	}
    }
    else
    {
	    wprintf(L"%ls%lcる\n", word, lastchr); //???
    }
    exit(EXIT_SUCCESS);
}

void de_form(wchar_t word[], int len)
{
    if (word[len - 1] == L'い')
    {
	word[len - 1] = L'\0';
	wprintf(L"%lsぐ\n", word);
    }
    else if (word[len - 1] == L'ん')
    {
	word[len - 1] = L'\0';
	wprintf(L"%lsむ\n", word);
	wprintf(L"%lsぶ\n", word);
	wprintf(L"%lsぬ\n", word);
    }
    exit(EXIT_SUCCESS);
}

void deinflectVerb(wchar_t input[]) {
    int len = wcslen(input);
    if (len < 3)
	return;

    // 形容詞 過去形
    else if (wcscmp(input + len - 3, L"かった") == 0)
    {
	input[len - 2] = L'\0';
	input[len - 3] = L'い';
	deinflectVerb(input);
	wprintf(L"%ls\n", input);
    }
    else if (wcscmp(input + len - 3, L"られる") == 0)
    {
	input[len - 3] = L'\0';
	wprintf(L"%lsる\n", input);
    }
    if (!wcscmp(input + len - 2,L"ない"))
    {
        input[len - 2] = L'\0';
	atouform(input, len - 2);
    }
    // ます and たい-form
    else if (!wcscmp(input + len - 2, L"ます") || !wcscmp(input + len - 2, L"たい"))
    {
        input[len - 2] = L'\0';
	itouform(input, len - 2);
    }
    else if (!wcscmp(input + len - 3, L"ません"))
    {
        input[len - 3] = L'\0';
	itouform(input, len - 3);
    }
    // 過去 + て-form
    if (input[len - 1] == L'て' || input[len - 1] == L'た')
    {
        input[len - 1] = L'\0';
	te_form(input, len - 1);
    }
    else if (input[len - 1] == L'で' || input[len - 1] == L'だ')
    {
        input[len - 1] = L'\0';
	de_form(input, len - 1);
    }
    else if (input[len - 1] == L'く')
    {
        input[len - 1] = L'\0';
	wprintf(L"%lsい\n", input);
    }

    return;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    setlocale(LC_ALL, "");

    wchar_t word[20];
    mbstowcs(word, argv[1], 20);

    deinflectVerb(word);

    return 0;
}

