#include <string>
#include "dokan/dokan.h"
#include "utils.h"
#include "filesystem_manager.h"
#include "read_operations.h"
#include "package_settings.h"

#ifdef WIN10_ENABLE_LONG_PATH
#define DOKAN_MAX_PATH 32768
#else
#define DOKAN_MAX_PATH MAX_PATH
#endif

using std::string;
using std::wstring;

bool is_filesystem_alive() { return true;}



#define IS_ROOT(x) (x == "\\")
#define IS_PATH_VALID(path) (path.find_last_of("/\\") == 0)

NTSTATUS DOKAN_CALLBACK
dokan_create_file(LPCWSTR wide_file_path, PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
				  ACCESS_MASK desired_access, ULONG file_attributes,
				  ULONG share_access, ULONG create_disposition,
				  ULONG create_options, PDOKAN_FILE_INFO dokan_file_info)
{
	DWORD creation_disposition;
	DWORD file_attributes_flags;
	ACCESS_MASK generic_desired_access;

	DokanMapKernelToUserCreateFileFlags(
		desired_access, file_attributes, create_options, create_disposition,
		&generic_desired_access, &file_attributes_flags, &creation_disposition);

	string file_path = wstringToString(wide_file_path);
	string file_name = get_file_name_in_path(file_path);
	filesystem_log("CreateFile : %s, ", file_path.c_str());

	//Check if the path matches what we expect
	if (!IS_PATH_VALID(file_path))
	{
		filesystem_log("access denied\n");
		return STATUS_ACCESS_DENIED;
	}

	// When filePath is a directory, needs to change the flag so that the file can
	// be opened.
	bool is_directory;
	if (IS_ROOT(file_path))
	{
		is_directory = TRUE;
		filesystem_log("path is a folder\n");

		if (create_options & FILE_NON_DIRECTORY_FILE)
		{
			filesystem_log("error: Path is a directory, but you open it as a file\n");
			return STATUS_FILE_IS_A_DIRECTORY;
		}
	}
	else
	{
		is_directory = FALSE;
		filesystem_log("path is a file\n");
		if (create_options & FILE_DIRECTORY_FILE)
		{
			filesystem_log("error: Path is a file, but you open it as a directory\n");
			return STATUS_NOT_A_DIRECTORY;
		}
		//Use cache
		dokan_file_info->Nocache = FALSE;
	}
	dokan_file_info->IsDirectory = is_directory;

	filesystem_log("FlagsAndAttributes = 0x%x\n", file_attributes_flags);
	filesystem_log("creation_disposition = 0x%x:\n", creation_disposition);

	NTSTATUS status = STATUS_ACCESS_DENIED;
	if (IS_ROOT(file_path) || file_list.has_key2(file_name))
	{
		if (creation_disposition == OPEN_ALWAYS)
		{
			status = STATUS_OBJECT_NAME_COLLISION;
		}
		if (creation_disposition == OPEN_EXISTING)
		{
			status = STATUS_SUCCESS;
		}
		if (creation_disposition == CREATE_NEW)
		{
			status = ERROR_FILE_EXISTS;
		}
		if (creation_disposition == CREATE_ALWAYS)
		{
			status = STATUS_ACCESS_DENIED;
		}
		if (creation_disposition == TRUNCATE_EXISTING)
		{
			status = STATUS_ACCESS_DENIED;
		}
	}
	else
	{
		if (creation_disposition == CREATE_NEW ||
			creation_disposition == CREATE_ALWAYS ||
			creation_disposition == TRUNCATE_EXISTING ||
			creation_disposition == OPEN_ALWAYS)
		{
			status = STATUS_ACCESS_DENIED;
		}
		if (creation_disposition == OPEN_EXISTING)
		{
			status = STATUS_NO_SUCH_FILE;
		}
	}
	filesystem_log("Return status:%d\n",status);
	return status;
}

void DOKAN_CALLBACK dokan_close_file(LPCWSTR wide_file_path,
									 PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	filesystem_log("CloseFile: %s\n", file_path.c_str());
}

void DOKAN_CALLBACK dokan_cleanup(LPCWSTR wide_file_path,
								  PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	filesystem_log("Cleanup: %s\n", file_path.c_str());
}

NTSTATUS DOKAN_CALLBACK dokan_read_file(LPCWSTR wide_file_path, LPVOID buffer,
										DWORD buffer_length,
										LPDWORD read_length,
										LONGLONG offset,
										PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	string file_name = get_file_name_in_path(file_path);
	filesystem_log("ReadFile: %s, ", file_path.c_str());

	filesystem_file_data &file_data = file_list.get_value_by_key2(file_name);
	size_t read_size = general_read_func(file_data, buffer, offset, buffer_length);
	filesystem_log("offset:%llu, request read %llu, true read:%llu\n", offset,buffer_length, read_size);
	*read_length = read_size;

	return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK dokan_write_file(LPCWSTR wide_file_path, LPCVOID Buffer,
										 DWORD NumberOfBytesToWrite,
										 LPDWORD NumberOfBytesWritten,
										 LONGLONG Offset,
										 PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	filesystem_log("WriteFile: %s\n", file_path.c_str());
	return ERROR_ACCESS_DENIED;
}

NTSTATUS DOKAN_CALLBACK
dokan_flush_buffer(LPCWSTR wide_file_path, PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	filesystem_log("FlushFileBuffers: %s\n", file_path.c_str());
	return STATUS_SUCCESS;
}

// my secret time: 09/17/2020 14:41
static FILETIME file_time{2711754096, 30837949};
NTSTATUS DOKAN_CALLBACK dokan_get_file_information(
	LPCWSTR wide_file_path, LPBY_HANDLE_FILE_INFORMATION HandleFileInformation,
	PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	string file_name = get_file_name_in_path(file_path);
	filesystem_log("GetFileInfo: %s\n", file_path.c_str());
	//HANDLE handle = (HANDLE)dokan_file_info->Context;
	if (!IS_PATH_VALID(file_path))
	{
		return STATUS_ACCESS_DENIED;
	}

	if (IS_ROOT(file_path))
	{
		HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
	}
	else
	{
		if (!file_list.has_key2(file_name))
		{
			return STATUS_NO_SUCH_FILE;
		}
		HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		filesystem_file_data &file_data = file_list.get_value_by_key2(file_name);
		HandleFileInformation->nFileSizeLow = (unsigned long)file_data.file_size;
		HandleFileInformation->nFileSizeHigh = file_data.file_size >> 32;
	}
	HandleFileInformation->ftCreationTime = file_time;
	HandleFileInformation->ftLastAccessTime = file_time;
	HandleFileInformation->ftLastWriteTime = file_time;
	HandleFileInformation->nNumberOfLinks = 1;
	return STATUS_SUCCESS;
}

static void copy_file_attris(LPWIN32_FIND_DATAW findData, LPBY_HANDLE_FILE_INFORMATION info)
{
	findData->nFileSizeHigh = info->nFileSizeHigh;
	findData->nFileSizeLow = info->nFileSizeLow;
	findData->dwFileAttributes = info->dwFileAttributes;
	findData->ftCreationTime = info->ftCreationTime;
	findData->ftLastAccessTime = info->ftLastAccessTime;
	findData->ftLastWriteTime = info->ftLastWriteTime;
}

NTSTATUS DOKAN_CALLBACK
MirrorFindFiles(LPCWSTR wide_file_path,
				PFillFindData FillFindData, // function pointer
				PDOKAN_FILE_INFO dokan_file_info)
{
	string file_path = wstringToString(wide_file_path);
	filesystem_log("FindFiles: %s\n", file_path.c_str());
	WIN32_FIND_DATAW findData;
	if (IS_ROOT(file_path))
	{
		BY_HANDLE_FILE_INFORMATION info;
		for (auto i = file_list.begin_key(); i != file_list.end_key(); ++i)
		{
			wstring wide_sub_file_path = stringToWstring(i->second);
			dokan_get_file_information((L"\\" + wide_sub_file_path).c_str(), &info, dokan_file_info);
			wcscpy(findData.cFileName, wide_sub_file_path.c_str());
			copy_file_attris(&findData, &info);
			filesystem_log("File Added: %s\n", i->second.c_str());
			FillFindData(&findData, dokan_file_info);
		}
		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_ACCESS_DENIED;
	}
}

static WCHAR mountpoint[DOKAN_MAX_PATH] = L"M:\\";
//[[Rcpp::export]]
void filesystem_thread_func()
{
	DOKAN_OPERATIONS dokanOperations;
	ZeroMemory(&dokanOperations, sizeof(DOKAN_OPERATIONS));
	dokanOperations.ZwCreateFile = dokan_create_file;
	dokanOperations.Cleanup = dokan_cleanup;
	dokanOperations.CloseFile = dokan_close_file;
	dokanOperations.ReadFile = dokan_read_file;
	dokanOperations.WriteFile = dokan_write_file;
	dokanOperations.FlushFileBuffers = dokan_flush_buffer;
	dokanOperations.GetFileInformation = dokan_get_file_information;
	dokanOperations.FindFiles = MirrorFindFiles;
	dokanOperations.FindFilesWithPattern = NULL;
	dokanOperations.SetFileAttributes = NULL;
	dokanOperations.SetFileTime = NULL;
	dokanOperations.DeleteFile = NULL;
	dokanOperations.DeleteDirectory = NULL;
	dokanOperations.MoveFile = NULL;
	dokanOperations.SetEndOfFile = NULL;
	dokanOperations.SetAllocationSize = NULL;
	dokanOperations.LockFile = NULL;
	dokanOperations.UnlockFile = NULL;
	dokanOperations.GetFileSecurity = NULL;
	dokanOperations.SetFileSecurity = NULL;
	dokanOperations.GetDiskFreeSpace = NULL;
	dokanOperations.GetVolumeInformation = NULL;
	dokanOperations.Unmounted = NULL;
	dokanOperations.FindStreams = NULL;
	dokanOperations.Mounted = NULL;

	wcscpy(mountpoint,stringToWstring(get_mountpoint()).c_str());
	DOKAN_OPTIONS dokanOptions;
	dokanOptions.MountPoint = mountpoint;
	dokanOptions.Version = 140;
	dokanOptions.ThreadCount = 1;
	//You CANNOT comment this out!
	dokanOptions.UNCName = L"";
	//dokanOptions.Timeout;
	//dokanOptions.AllocationUnitSize;
	//dokanOptions.SectorSize;
	dokanOptions.Options = 0;

	int status = DokanMain(&dokanOptions, &dokanOperations);
	filesystem_log("Exist main function, status: %d", status);
}


void filesystem_stop() {
	DokanRemoveMountPoint(mountpoint);
}