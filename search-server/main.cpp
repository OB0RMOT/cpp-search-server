// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <vector>

using namespace std;

int main()
{
    int a = 0;
    int tmp2, j;
    vector<int> x;
    for (int i = 1; i <= 1000; ++i)
    {
        x.push_back(i);
    }
    for (int i : x)
    {
        if (i > 9)
        {
            j = i;
            tmp2 = i;
            while (tmp2 > 9)
            {
                j = tmp2 % 10;
                tmp2 = tmp2 / 10;
                if ((tmp2 == 3) || (j == 3))
                {
                    ++a;
                    break;
                }
            }
        }
    }
    cout << a + 1;

    getchar();
}
// Закомитьте изменения и отправьте их в свой репозиторий.
