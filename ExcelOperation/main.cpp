#include <iostream>
#include "ExcelOperation.h"

using namespace std;

int main()
{
	if (S_OK != CoInitialize(NULL))//init
	{
		cout<<"��ʼ��oleʧ��"<<endl;
		return -1;
	}

	ExcelOperation excel;
	if (excel.OpenFromTemplate(_T("E:\\test_by.xlsx")))
	{
		excel.Test1();
		excel.SaveAsPDF(_T("E:\\test_by.pdf"));
	}
	excel.Close();

	CoUninitialize();//Uninitialize
	return 0;
}