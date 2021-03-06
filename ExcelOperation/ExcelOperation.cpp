#include "ExcelOperation.h"
#include <comdef.h>
#include <iostream>

using namespace std;

ExcelOperation::ExcelOperation(void)
:covTrue((short)TRUE)
,covFalse((short)FALSE)
,covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR)
{
}

ExcelOperation::~ExcelOperation(void)
{
	Close();
}

bool ExcelOperation::CheckVersion()
{
	try
	{
		CString version = m_app.get_Version();
		//2010版 版本号为"14.0"; 2013版 版本号为"15.0"
		if (version == _T("14.0") || version == _T("15.0"))
		{
			return true;
		}
	}
	catch (CException* e)
	{
		cout<<"版本号检查异常."<<endl;
		return false;
	}
	return false;
}

void ExcelOperation::Close()
{
	m_workbook.Close(covFalse,covOptional,covOptional);//默认不保存更改
	m_workbooks.Close();
	m_range.ReleaseDispatch();
 	m_worksheet.ReleaseDispatch();
 	m_worksheets.ReleaseDispatch();
 	m_workbook.ReleaseDispatch();
 	m_workbooks.ReleaseDispatch();
	m_app.Quit();
	m_app.ReleaseDispatch();
}

bool ExcelOperation::CreateTwoDimSafeArray(COleSafeArray &safeArray, DWORD dimElements1, 
										 DWORD dimElements2, long iLBound1, long iLBound2)
{
	try
	{
		const int dim = 2;
		SAFEARRAYBOUND saBound[dim];
		saBound[0].cElements = dimElements1;
		saBound[0].lLbound = iLBound1;
		saBound[1].cElements = dimElements2;
		saBound[1].lLbound = iLBound2;

		safeArray.Create(VT_VARIANT, dim, saBound);
	}
	catch (CException* e)
	{
		cout<<"创建二维数组异常."<<endl;
		return false;//创建二维数组失败
	}
	return true;
}

bool ExcelOperation::GetCellsValue(COleVariant startCell,COleVariant endCell, VARIANT &iData)
{
	try
	{
		m_range.AttachDispatch(m_worksheet.get_Range(startCell, endCell));
		iData = m_range.get_Value2();
	}
	catch (CException* e)
	{
		cout<<"获取单元格区域数据异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::GetCellValue(COleVariant rowIndex, COleVariant columnIndex,VARIANT &data)
{
	try
	{
		m_range.AttachDispatch(m_worksheet.get_Cells());
		data = m_range.get_Item(rowIndex, columnIndex);
	}
	catch (CException* e)
	{
		cout<<"获取单元格数据异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::IsFileExist(const CString &fileName)
{
	DWORD dwAttrib = GetFileAttributes(fileName);
	return INVALID_FILE_ATTRIBUTES != dwAttrib;
}

bool ExcelOperation::IsRegionEqual(CRange &iRange,COleSafeArray &iTwoDimArray)
{
	if ( iTwoDimArray.GetDim() != 2)//数组维数判断
	{
		return false;//数组非二维数组
	}

	//获取二维数组行数和列数
	long firstLBound = 0;//第一维左边界
	long fristUBound = 0;//第一维右边界
	long secondLBound = 0;//第二维左边界
	long secondUBound = 0;//第二维右边界
	long row1 = 0;//行数
	long column1 = 0;//列数
	iTwoDimArray.GetLBound(1,&firstLBound);
	iTwoDimArray.GetUBound(1,&fristUBound);
	iTwoDimArray.GetLBound(2,&secondLBound);
	iTwoDimArray.GetUBound(2,&secondUBound);
	row1 = fristUBound - firstLBound + 1;
	column1 = secondUBound - secondLBound + 1;

	//获取表格区域行数和列数
	CRange range;
	range.AttachDispatch(iRange.get_Rows());
	long row2 = range.get_Count();//获取行数
	range.AttachDispatch(iRange.get_Columns());
	long column2 = range.get_Count();//获取列数
	if (row1 != row2 || column1 != column2)
	{
		return false;//区域大小不一致
	}
	return true;
}

bool ExcelOperation::Open(const CString &iFileName, bool autoCreate)
{
	try
	{
		if (!m_app.CreateDispatch(_T("Excel.Application")))//创建Excel服务
		{
			cout<<"创建Excel服务失败."<<endl;
			return false;
		}
		if (!CheckVersion())
		{
			cout<<"Excel版本过低不符合要求,请检查."<<endl;
			return false;
		}
		m_app.put_Visible(FALSE);//默认不显示excel表格

		m_workbooks.AttachDispatch(m_app.get_Workbooks());
		if (IsFileExist(iFileName))
		{
			m_workbook.AttachDispatch(m_workbooks.Open(iFileName,vtMissing,vtMissing,
				vtMissing,vtMissing,vtMissing,vtMissing,vtMissing,vtMissing,vtMissing,
				vtMissing,vtMissing,vtMissing,vtMissing,vtMissing));//获取workbook
		}
		else if (autoCreate)
		{
			m_workbook.AttachDispatch(m_workbooks.Add(COleVariant(covOptional)));//获取workbook
			SaveAs(iFileName);//创建新文件
		}
		else
		{
			cout<<"excel文件不存在."<<endl;
			return false;
		}
		m_worksheets.AttachDispatch(m_workbook.get_Sheets());//获取所有sheet
		m_worksheet.AttachDispatch(m_workbook.get_ActiveSheet());//获取当前活动sheet
	}
	catch (CException* e)
	{
		cout<<"excel文件打开异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::OpenFromTemplate(const CString &iFileName)
{
	try
	{
		if (!m_app.CreateDispatch(_T("Excel.Application")))//创建Excel服务
		{
			cout<<"创建Excel服务失败."<<endl;
			return false;
		}
		if (!CheckVersion())
		{
			cout<<"Excel版本过低."<<endl;
			return false;
		}
		m_app.put_Visible(FALSE);//默认不显示excel表格

		if (!IsFileExist(iFileName))
		{
			cout<<"excel文件不存在."<<endl;
			return false;
		}
		m_workbooks.AttachDispatch(m_app.get_Workbooks());
		//以路径指出的文件为模板创建excel
		m_workbook.AttachDispatch(m_workbooks.Add(COleVariant(iFileName)));//获取workbook
		m_worksheets.AttachDispatch(m_workbook.get_Sheets());//获取所有sheet
		m_worksheet.AttachDispatch(m_workbook.get_ActiveSheet());//获取当前活动sheet
	}
	catch (CException* e)
	{
		cout<<"excel文件打开异常."<<endl;
		return false;
	}
	
	return true;
}

bool ExcelOperation::Save()
{
	try
	{
		m_workbook.Save();
	}
	catch (CException* e)
	{
		cout<<"文件保存异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::SaveAs(const CString &iFileName)
{
	try
	{
		long acessMode = 1;//默认值（不更改访问模式）
		m_workbook.SaveAs(COleVariant(iFileName),covOptional,covOptional,covOptional,covOptional,covOptional
			,acessMode,covOptional,covOptional,covOptional,covOptional,covOptional);
	}
	catch (CException* e)
	{
		cout<<"文件另存异常."<<endl;
		return false ;
	}
	return true;
}

bool ExcelOperation::SaveAsPDF(const CString &iFileName)
{
	try
	{
		long type = 0;//参数0为.pdf，参数1为.xps
		m_workbook.ExportAsFixedFormat(type,COleVariant(iFileName),covOptional,covOptional,
			covOptional,covOptional,covOptional,covOptional,covOptional);
	}
	catch (CException* e)
	{
		cout<<"文件另存为pdf异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::SetCellsValue(COleVariant startCell,COleVariant endCell,COleSafeArray &iTwoDimArray)
{
	try
	{
		m_range.AttachDispatch(m_worksheet.get_Range(COleVariant(startCell),COleVariant(endCell)));
		if (!IsRegionEqual(m_range,iTwoDimArray))
		{
			return false;
		}
		m_range.put_Value2(iTwoDimArray);
	}
	catch (CException* e)
	{
		cout<<"写入单元格区域数据异常."<<endl;
		return false;
	}
	
	return true;
}

bool ExcelOperation::SetCellValue(COleVariant rowIndex, COleVariant columnIndex,COleVariant data)
{
	try
	{
		m_range.AttachDispatch(m_worksheet.get_Cells());
		m_range.put_Item(rowIndex,columnIndex,data);
	}
	catch (CException* e)
	{
		cout<<"写入单元格数据异常."<<endl;
		return false;
	}
	return true;
}

bool ExcelOperation::SwitchWorksheet(const CString &sheetName)
{
	try
	{
		m_worksheet.AttachDispatch(m_worksheets.get_Item(COleVariant(sheetName)));//获取sheet
	}
	catch (CException* e)
	{
		cout<<"切换sheet异常."<<endl;
		return false;
	}
	return true;
}

void ExcelOperation::Test1()
{
	//获取数据
	COleSafeArray safeArray;
	GetCellsValue(_T("$A$1"),_T("$Z$35"),safeArray);
	long firstLBound = 0;//第一维左边界
	long firstUBound = 0;//第一维右边界
	long secondLBound = 0;//第二维左边界
	long secondUBound = 0;//第二维右边界
	safeArray.GetLBound(1,&firstLBound);
	safeArray.GetUBound(1,&firstUBound);
	safeArray.GetLBound(2,&secondLBound);
	safeArray.GetUBound(2,&secondUBound);

	long index[2] = {0};
	for (int i = firstLBound; i <= firstUBound; ++i)
	{
		index[0] = i;
		for (int j = secondLBound; j <= secondUBound; ++j)
		{
			index[1] = j;
			VARIANT var;
			safeArray.GetElement(index,&var);
			if (var.vt == VT_BSTR)
			{
				var = COleVariant(_T("str"));
			}
			safeArray.PutElement(index,&var);
		}
	}

	SetCellsValue(_T("$A$1"),_T("$Z$35"),safeArray);

	CPageSetup pageSetUp;
	pageSetUp.AttachDispatch(m_worksheet.get_PageSetup());
	long xlLandscape = 2;//横向模式
	pageSetUp.put_Orientation(xlLandscape);
	//pageSetUp.put_PrintArea(_T("$I$25:$P$31,$J$35:$M$41,$O$35:$R$41"));
	//pageSetUp.put_PrintArea(_T("A15:G30"));

// 	m_range.AttachDispatch(m_worksheet.get_Cells());
// 	VARIANT var = COleVariant(_T("mill"));
// 
// 	LPDISPATCH lpDisp = m_range.Find(var,vtMissing,vtMissing,
// 		vtMissing,vtMissing,1,vtMissing,vtMissing,vtMissing);
// 	
// 	if (lpDisp != NULL)
// 	{
// 		m_range.AttachDispatch(lpDisp);
// 		CString str = m_range.get_Address(vtMissing,vtMissing,2,vtMissing,vtMissing);
// 		MessageBox(NULL,str,NULL,MB_OK);
// 	}
// 	else
// 	{
// 		MessageBox(NULL,_T("没找到"),NULL,MB_OK);
// 	}
}

CRange * ExcelOperation::GetRange()
{
	return &m_range;
}

CWorkbook * ExcelOperation::GetWorkbook()
{
	return &m_workbook;
}

CWorkbooks * ExcelOperation::GetWorkbooks()
{
	return &m_workbooks;
}

CWorksheet * ExcelOperation::GetWorksheet()
{
	return &m_worksheet;
}

CWorksheets * ExcelOperation::GetWorksheets()
{
	return &m_worksheets;
}