/*
 *
 *  The Sleuth Kit
 *
 *  Contact: Brian Carrier [carrier <at> sleuthkit [dot] org]
 *  Copyright (c) 2010-2013 Basis Technology Corporation. All Rights
 *  reserved.
 *
 *  This software is distributed under the Common Public License 1.0
 */

/**
 * \file
 * 
 */

#include <iostream>
#include <sstream>
#include <algorithm>

#include "TskL01Extract.h"
//#include "TskAutoImpl.h"
#include "Services/TskServices.h"
#include "tsk3/base/tsk_base_i.h"

//namespace ewf
//{
//    #include "ewf.h"
//}


TskL01Extract::TskL01Extract() :
    m_db(TskServices::Instance().getImgDB())
{
    m_img_info = NULL;
    m_images_ptrs = NULL;
}

TskL01Extract::~TskL01Extract()
{
    close();
}

/**

*/
int TskL01Extract::open(const TSK_TCHAR *imageFile)
{
    if (!m_images.empty()) {
        close();        
    }
    m_images.push_back(imageFile);
    return openContainers();
}

int TskL01Extract::open(const std::vector<std::wstring> &images)
{
    return -1;
}

/*
 * Opens the image files listed in ImgDB for later analysis and extraction.  
 * @returns -1 on error and 0 on success
 */
int TskL01Extract::open()
{
    return -1;
}


int TskL01Extract::openContainers()
{
    m_images_ptrs = (const wchar_t **)malloc(m_images.size() * sizeof(wchar_t *));
    if (m_images_ptrs == NULL)
        return -1;

    int i = 0;
    for(std::vector<std::wstring>::iterator list_iter = m_images.begin(); 
        list_iter != m_images.end(); list_iter++)
    {
            m_images_ptrs[i++] = (*list_iter).c_str();
    }

    m_img_info = tsk_img_open(i, m_images_ptrs, TSK_IMG_TYPE_EWF_EWF, 512);
    if (m_img_info == NULL) 
    {
        std::wstringstream logMessage;
        logMessage << L"TskL01Extract::openContainers - Error with tsk_img_open: " << tsk_error_get() << std::endl;
        LOGERROR(logMessage.str());
        return -1;
    }

    ewf::IMG_EWF_INFO *ewfInfo = (ewf::IMG_EWF_INFO*)m_img_info;

    ewf::libewf_file_entry_t *root = NULL;
    ewf::libewf_error_t *ewfError = NULL;
    int ret = ewf::libewf_handle_get_root_file_entry(ewfInfo->handle, &root, &ewfError);
    if (ret == -1)
    {
        std::wstringstream logMessage;
        char errorString[512];
        errorString[0] = '\0';
        ewf::libewf_error_backtrace_sprint(ewfError, errorString, 512);
        logMessage << L"TskL01Extract::openContainers - Error with libewf_handle_get_root_file_entry: " << errorString << std::endl;
        LOGERROR(logMessage.str());
        return -1;
    }

    if (ret > 0)
    {
        ewf::uint8_t nameString[512];
        nameString[0] = '\0';
        ewfError = NULL;
        if (ewf::libewf_file_entry_get_utf8_name(root, nameString, 512, &ewfError) == -1)
        {
            std::wstringstream logMessage;
            char errorString[512];
            errorString[0] = '\0';
            ewf::libewf_error_backtrace_sprint(ewfError, errorString, 512);
            logMessage << L"TskL01Extract::openContainers - Error with libewf_file_entry_get_utf8_name: " << errorString << std::endl;
            LOGERROR(logMessage.str());
            return -1;
        }

       
        traverse(root, 0);
    }

    /// dev testing ////
    return -1;
    //return 0;
}


void TskL01Extract::traverse(ewf::libewf_file_entry_t *parent, int index)
{
    std::cerr << "traversing child " << index << std::endl;
  
    ewf::libewf_error_t *ewfError = NULL;

    int num = 0;
    ewf::libewf_file_entry_get_number_of_sub_file_entries(parent, &num, &ewfError);

    if (num > 0)
    {
        ewf::libewf_file_entry_t *child = NULL;

        if (ewf::libewf_file_entry_get_sub_file_entry(parent, index, &child, &ewfError) == -1)
        {
            std::wstringstream logMessage;
            char errorString[512];
            errorString[0] = '\0';
            ewf::libewf_error_backtrace_sprint(ewfError, errorString, 512);
            logMessage << L"TskL01Extract::openContainers - Error with libewf_file_entry_get_sub_file_entry: " << errorString << std::endl;
            LOGERROR(logMessage.str());
        }
        
        printName(child);
        std::cerr << "number of sub file entries = " << num << std::endl;

        //recurse
        for (int i=0; i < num; ++i)
        {
            traverse(child, i);
        }
    }
}


void TskL01Extract::printName(ewf::libewf_file_entry_t *node)
{
    ewf::uint8_t nameString[512];
    nameString[0] = '\0';
    ewf::libewf_error_t *ewfError = NULL;
    if (ewf::libewf_file_entry_get_utf8_name(node, nameString, 512, &ewfError) == -1)
    {
        std::wstringstream logMessage;
        char errorString[512];
        errorString[0] = '\0';
        ewf::libewf_error_backtrace_sprint(ewfError, errorString, 512);
        logMessage << L"TskL01Extract::openContainers - Error with libewf_file_entry_get_utf8_name: " << errorString << std::endl;
        LOGERROR(logMessage.str());
        return;
    }
    std::cerr << "L01 node file name = " << nameString << std::endl;
}

void TskL01Extract::close()
{
    if (m_img_info) {
        tsk_img_close(m_img_info);
        m_img_info = NULL;
    }

    if (m_images_ptrs) {
        free(m_images_ptrs);
        m_images_ptrs = NULL;
    }

    m_images.clear();
}

/*
 * @param start Sector offset to start reading from in current sector run
 * @param len Number of sectors to read
 * @param a_buffer Buffer to read into (must be of size a_len * 512 or larger)
 * @returns -1 on error or number of sectors read
 */
int TskL01Extract::getSectorData(const uint64_t sect_start, 
                                const uint64_t sect_len, 
                                char *buffer)
{
    return -1;
}

/*
 * @param byte_start Byte offset to start reading from start of file
 * @param byte_len Number of bytes to read
 * @param buffer Buffer to read into (must be of size byte_len or larger)
 * @returns -1 on error or number of bytes read
 */
int TskL01Extract::getByteData(const uint64_t byte_start, 
                                const uint64_t byte_len, 
                                char *buffer)
{
    return -1;
}

int TskL01Extract::extractFiles()
{

    return 0;
}

int TskL01Extract::openFile(const uint64_t fileId)
{
    return -1;

}

int TskL01Extract::readFile(const int handle, 
                              const uint64_t byte_offset, 
                              const size_t byte_len, 
                              char * buffer)
{
    return -1;
}

int TskL01Extract::closeFile(const int handle)
{
    return -1;
}
