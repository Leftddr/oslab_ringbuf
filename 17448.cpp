#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
using namespace std;

int *arr;
int *result;
int *cri;
int max1,num;
int max_profit=-99999;

void dfs(int,int);

int main(){
    char ch[100];
    fgets(ch,sizeof(ch),stdin);

    char *ptr=strtok(ch," ");
    num=atoi(ptr);
    ptr=strtok(NULL," ");
    max1=atoi(ptr);

    arr=new int[num];
    cri=new int[num];
    result=new int[num];

    fgets(ch,sizeof(ch),stdin);
    ptr=strtok(ch," ");
    int cnt=0;
    while(ptr!=NULL){
        cri[cnt]=0;
        arr[cnt++]=atoi(ptr);
        ptr=strtok(NULL," ");
    }
    //printf("a");

    cnt=0;
    for(int i=0;i<num;i++){
        if(max1-1>=0){
            cri[i]=1;
            result[cnt++]=arr[i]*(-1);
            dfs(cnt,max1-1);
            cnt--;
            cri[i]=0;
        }
        if(max1-2>=0){
            cri[i]=1;
            result[cnt++]=arr[i];
            dfs(cnt,max1-2);
            cnt--;
            cri[i]=0;
        }
    }
    delete []result;
    delete []arr;
    delete []cri;
    printf("%d\n",max_profit);

}

void dfs(int cnt,int left){
    if(left==0){
        int sum=1;
        for(int i=0;i<cnt;i++){
            printf("%d ",result[i]);
            sum*=result[i];
        }
        printf("\n");
        if(sum>max_profit){max_profit=sum;}
    }
    else{
        for(int i=0;i<num;i++){
            if(cri[i]==1){continue;}
            if(left-1>=0){
                result[cnt++]=arr[i]*(-1);
                cri[i]=1;
                dfs(cnt,left-1);
                cnt--;
                cri[i]=0;
            }
            if(left-2>=0){
                result[cnt++]=arr[i];
                cri[i]=1;
                dfs(cnt,left-2);
                cnt--;
                cri[i]=0;
            }
        }
    }
}