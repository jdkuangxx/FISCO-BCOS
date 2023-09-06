#ifndef __MACROS_COMMON_H
#define __MACROS_COMMON_H

#define DISALLOW_COPY(_class_) _class_(const _class_&) = delete
#define DISALLOW_ASSIGN(_class_) _class_& operator=(const _class_&) = delete
#define DISALLOW_COPY_AND_ASSIGN(_class_) DISALLOW_COPY(_class_); DISALLOW_ASSIGN(_class_)    

#define CACHE_LINE_SIZE 64

#endif