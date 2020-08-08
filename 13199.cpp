#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define max 4
int arr[max];

int main(){
    char ch[128];
    printf("input four argc : ");
    fgets(ch,sizeof(ch),stdin);

    char *ptr=strtok(ch," ");
    int cnt=0;
    while(ptr!=NULL){
        arr[cnt++]=atoi(ptr);
        ptr=strtok(NULL," ");
    }

    int coupon1;
    int coupon2;
    coupon1=coupon2=arr[1]/arr[0];
    int result1;
    int result2;
    result1=result2=coupon1;

    while(coupon2>=arr[2]){
        result1+=coupon1/arr[2];
        coupon1%=arr[2];
        result2+=coupon2/arr[2];
        coupon2=(coupon2%arr[2])+(coupon2/arr[2]);
    }

    printf("%d\n",result2-result1);
    
}