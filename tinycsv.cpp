#include<string>//Tiny CSV pretty-print
#include<vector>//Matthew Chandler
#include<cstdio>//mattvchandler@gmail
#define E feof(f)//©2019 by MIT license
#define G c=getc(f);//build w/ c++17
#define Z size()//./tinycsv foo.csv
using namespace std;using I=int;using S=
string;pair<S,I>p(FILE*f){if(E)return{{}
,0};I q=0,e=0,c;S w;while(1){G if(c==EOF
&&!E)throw 0;if(c=='"'){if(q){G if(c==
','||c=='\n'||c=='\r'||E)q=0;else if(c!=
'"')throw 0;}else{if(w.Z<1){q=1;continue
;}else throw 0;}}if(E&&q)throw 0;else if
(!q&&c==',')break;else if(!q&&(c=='\n'||
c=='\r'||E)){e=1;while(!E){G if(c!='\r'
&&c!='\n'){if(!E)ungetc(c,f);break;}}
break;}w+=c;}return{w,e};}I main(I c,
char*v[]){if(c!=2)return 1;auto f=fopen(
v[1],"r");try{if(!f)throw 0;vector<
vector<S>>d(1);vector<I>z;for(I i=0;;++i
){const auto&[w,e]=p(f);d.back().
push_back(w);z.resize(max(i,(I)w.Z));z[i
]=max(z[i],(I)w.Z);if(E)break;if(e)d.
push_back({});}for(auto&r:d){for(I i=0;i
<r.Z;++i){if(i!=0)printf(" | ");printf(
"%-*s",z[i],data(r[i]));}putchar('\n');}
}catch(I){puts("error");return 1;}}
