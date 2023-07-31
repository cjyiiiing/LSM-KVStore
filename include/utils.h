#ifndef LSMKVSTORE_UTILS_H_
#define LSMKVSTORE_UTILS_H_

#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sstream>
#include <unistd.h>

namespace utils {

/**
 * @brief 扫描目录下的所有文件
 * @param[in] path 目录路径
 * @param[out] ret 该目录下所有文件的名字
 * @return int 文件数量
*/
inline int ScanDir(const std::string path, std::vector<std::string> &ret) {
    struct dirent **namelist;
    char s[100];        // 保存读取的文件名
    int file_num = scandir(path.c_str(), &namelist, 0, alphasort);

    if (file_num > 0) {
        int index=0;
        while (index < file_num) {
            strcpy(s, namelist[index]->d_name);
            if (s[0] != '.') {
                ret.push_back(s);
            }
            // printf("d_ino: %ld  d_off:%ld d_name: %s\n", namelist[index]->d_ino,namelist[index]->d_off,namelist[index]->d_name);
            free(namelist[index]);
            index++;
        }
        free(namelist);
    }

    return ret.size();
}
// inline int ScanDir(const std::string path, std::vector<std::string> &ret) {
//     DIR *dir;       // 不透明的目录流结构体类型，表示打开的目录
//     dir = opendir(path.c_str());
//     struct dirent *ent;          // 表示目录中的一个文件或者子目录
//     char s[100];        // 保存读取的文件名
//     while ((ent = readdir(dir)) != NULL) {
//         strcpy(s, ent->d_name);
//         if (s[0] != '.') {
//             ret.push_back(s);
//         }
//     }
//     closedir(dir);
//     return ret.size();   // 存在的问题：ret没有排序
// }

/**
 * @brief 判断目录是否存在
 * @param[in] path 要判断的目录
 * @return true存在，false不存在
 */
bool DirExists(std::string path) {
    struct stat st;
    int ret = stat(path.c_str(), &st);
    return (ret == 0) && (S_ISDIR(st.st_mode));
}

/**
 * @brief 创建目录，-rwxrwxr-x
 * @param[in] path 要创建的目录
 * @return 成功返回0，失败返回-1
 */
inline int _MkDir(const char *path) {
    return ::mkdir(path, 0775);
}

/**
 * @brief 递归地创建目录
 * @param[in] path 目录路径
 * @return true成功，false失败
*/
inline bool MkDir(const char *path) {
    std::string cur_path = "";
    std::stringstream ss(path);
    std::string dir_name;
    while (std::getline(ss, dir_name, '/')) {
        cur_path += dir_name;
        if (!DirExists(cur_path) && _MkDir(cur_path.c_str()) != 0) {
            return false;
        }
        cur_path += "/";
    }
    return true;
}

/**
 * @brief 删除一个文件
 * @param[in] path 要删除的文件路径
 * @return 成功返回0，失败返回-1
 */
inline int RmFile(const char *path) {
    return ::unlink(path);
}

/**
 * @brief 删除一个空文件夹
 * @param[in] path 要删除的空文件夹路径
 * @return 成功返回0，失败返回-1
 */
inline int RmDir(const char *path) {
    return ::rmdir(path);
}

};

#endif // !LSMKVSTORE_UTILS_H_