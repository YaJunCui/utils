//********************************************
//	Ini 相关函数
//********************************************

#ifndef _INI_H_
#define _INI_H_

#ifndef SAFE_DELETE
#define SAFE_DELETE(x)	if( (x)!=NULL ) { delete (x); (x)=NULL; }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x)	if( (x)!=NULL ) { delete[] (x); (x)=NULL; }
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x)	if( (x)!=NULL ) { free(x); (x)=NULL; }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)	if( (x)!=NULL ) { (x)->Release(); (x)=NULL; }
#endif

#define ERROR_DATA -99999999

//配置文件类
class CIni
{
////////////////////////////////////////////////
// 内部数据
////////////////////////////////////////////////
private:
	char m_strFileName[MAX_PATH];	//文件名
	long m_lDataLen;				//文件长度
	char* m_strData;				//文件内容

	int IndexNum;					//索引数目（[]的数目）
	int *IndexList;					//索引点位置列表
	int Point;						//当前指针
	int Line, Word;					//当前行列

////////////////////////////////////////////////
// 通用接口
////////////////////////////////////////////////
public:
	CIni();
	CIni(char*);							//初始化打开配置文件
	virtual ~CIni();						//释放内存
	char *GetData();						//返回文件内容
	int GetLines(int);						//返回文件的行数

	bool Open(char *);						//打开配置文件
	void Close();							//关闭配置文件
	bool Save(char *filename=NULL);			//保存配置文件

////////////////////////////////////////////////
// 内部函数
////////////////////////////////////////////////
private:
	void InitIndex();						//初始化索引
	int FindIndex(char *);					//返回标题位置
	int FindData(int, char *);				//返回数据位置
	int GotoNextLine(int); 					//提行
	char *ReadDataName(int &);				//在指定位置读一数据名称
	char *ReadText(int);					//在指定位置读字符串

	bool AddIndex(char *);					//加入一个索引
	bool AddData(int, char *, char *);		//在当前位置加入一个数据
	bool ModityData(int, char *, char *);	//在当前位置修改一个数据的值
	int GotoLastLine(char *index);			//把指针移动到本INDEX的最后一行

////////////////////////////////////////////////
// 公用接口
////////////////////////////////////////////////
public:
	int ReadInt(char *, char *);			//读一个整数
	int ReadInt(char *, int );				//在指定的行读一整数
	char *ReadText(char *, char *);			//读一个字符串
	char *ReadText(char *, int);			//在指定的行读一字符串
	char *ReadCaption(char *, int);			//在指定行读一字符名称
	bool Write(char *, char *, int);		//写一个整数
	bool Write(char *, char *, char *);		//写一个字符串
	int GetContinueDataNum(char *);			//返回连续的行数（从INDEX到第一个空行）
};

#endif