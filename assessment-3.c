#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORDS 1000
#define MAX_LEN 101

typedef struct{
    char word[MAX_LEN];
    int count;
} WordEntry;

int is_word_char(char c){
    return isalnum(c);
}

void to_lower(char *str){
    for(int i = 0; str[i]; i++){
        str[i] = tolower(str[i]);
    }
}

int find_word(WordEntry words[], int total, const char *target){
    for (int i = 0; i<total; i++){
        if(strcmp(words[i].word, target) == 0){
            return i;
        }

    }
    return -1;

    }

int main(){

    FILE *input = fopen("input.txt", "r");
    if(!input){
        printf("Error: input.txt not found.\n");
        return 1;
    }

    WordEntry words[MAX_WORDS];
    int total = 0;
    char buffer[MAX_LEN];
    int len = 0;
    char ch;


    while((ch = fgetc(input)) != EOF){
        if(is_word_char(ch)){
            if(len<MAX_LEN -1){
                buffer[len++] = ch;
            }
        }else if(len>0){
            buffer[len] = '\0';
            to_lower(buffer);

            int index = find_word(words, total, buffer);
            if(index != -1){
                words[index].count++;
            }else if(total < MAX_WORDS){
                strcpy(words[total].word, buffer);
                words[total].count = 1;
                total++;
            }
            len = 0;
        }
    }



    if(len>0){
        buffer[len] = '\0';
        to_lower(buffer);

        int index = find_word(words, total, buffer);
        if(index != -1){
            words[index].count++;
        }else if(total <MAX_WORDS){
            strcpy(words[total].word, buffer);
            words[total].count = 1;
            total++;
        }
    }



    fclose(input);
    char result[MAX_LEN] = "";
    int maxCount = 0;

    for(int i = 0; i<total; i++){
        if(words[i].count > maxCount || (words[i].count == maxCount && strcmp(words[i].word, result)<0)){
            maxCount = words[i].count;
            strcpy(result, words[i].word);
        }
    }


    FILE *output = fopen("output.txt", "w");
    if(!output){
        printf("Error: could not create output.txt\n");
        return 1;
    }

    if(maxCount > 0){
        fprintf(output, "%s %d\n", result, maxCount);
        printf("Result from output.txt: \n%s %d\n", result, maxCount);
    }else{
        fprintf(output, "No valid words found.\n");
        printf("No valid words found.\n");
    }

    fclose(output);

    return 0;
}




