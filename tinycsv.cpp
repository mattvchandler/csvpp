#include<string>
#include<vector>
#include<cstdio>
#define B emplace_back();
#define E feof(f)
#define G c=getc(f);
#define Z size()
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
vector<S>>d(1);vector<I>z;while(1){const
auto&[w,e]=p(f);d.back().push_back(w);if
(z.Z<d.back().Z)z.B z[d.back().Z-1]=max(
z[d.back().Z-1],(I)w.Z);if(E)break;if(e)
d.B}for(auto&r:d){for(I i=0;i<(I)r.Z;++i
){if(i!=0)printf(" | ");printf("%-*s",(I
)z[i],r[i].c_str());}putchar('\n');}}
catch(I){puts("error");return 1;}}