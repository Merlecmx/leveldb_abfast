/* Copyright 2020 Google Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
      http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstddef>
// #include <filesystem>  // error with it when compiling
#include <experimental/filesystem>
#include <memory>
#include <string>
#include <dirent.h>

#include "leveldb/db.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"
#include "leveldb/status.h"

using namespace std;

static char kDbPath[] = "/tmp/testdb";


// Returns nullptr (a falsey unique_ptr) if opening fails.
std::unique_ptr<leveldb::DB> OpenDB() {
  leveldb::Options options;
  options.create_if_missing = true;

  leveldb::DB* db_ptr;
  leveldb::Status status =
      leveldb::DB::Open(options, kDbPath, &db_ptr);
  if (!status.ok())
    return nullptr;

  return std::unique_ptr<leveldb::DB>(db_ptr);
}

enum FuzzOp {
  kPut = 0,
  kGet = 1,
  kDelete = 2,
  kGetProperty = 3,
  kIterate = 4,
  kGetReleaseSnapshot = 5,
  kReopenDb = 6,
  kCompactRange = 7,cd
  // Add new values here.

};

size_t countDiskUsage(const char* pathname)
{
  if (pathname == NULL) {
    printf("Erorr: pathname is NULL\n");
  }

  struct stat stats;

  if (lstat(pathname, &stats) == 0) {
    if (S_ISREG(stats.st_mode)){
      return stats.st_size;
    }
  } else {
    perror("lstat\n");
  }

  DIR* dir = opendir(pathname);

  if (dir == NULL) {
    perror("Error");
    return 0;
  }

  struct dirent *dirEntry;
  size_t totalSize = 4096;

  for (dirEntry = readdir(dir); dirEntry != NULL; dirEntry =   readdir(dir)) {
    long pathLength = sizeof(char) * (strlen(pathname) + strlen(dirEntry->d_name) + 2);
    char* name = (char*)malloc(pathLength);
    strcpy(name, pathname);
    strcpy(name + strlen(pathname), "/");
    strcpy(name + strlen(pathname) + 1, dirEntry->d_name);

    if (dirEntry->d_type == DT_DIR) {
      if (strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {
        totalSize += countDiskUsage(name);
      }
    } else {
      int status = lstat(name, &stats);
      if (status == 0) {
        totalSize += stats.st_size;
      } else {
        perror("lstat\n");
      }
    }
    free(name);
  }

  closedir(dir);

  return totalSize;
}

int main(){
	
	// If the size of dir "/tmp/testdb" is over 100MB, then delete it in case chaos of the dir
	if(!access(kDbPath, F_OK) && countDiskUsage(kDbPath) > 100*1024*1024){
		
		std::experimental::filesystem::remove_all(kDbPath);
	}

	char key_i[1000000], value_i[1000000], name_i[1000000], begin_key_i[1000000], end_key_i[1000000];
	int opt;
	scanf("%d\r\n", &opt);
	scanf("%s\r\n", key_i);
	scanf("%s\r\n", value_i);
	scanf("%s\r\n", name_i);
	scanf("%s\r\n", begin_key_i);
	scanf("%s\r\n", end_key_i);

	std::unique_ptr<leveldb::DB> db = OpenDB();
  	if (!db.get())
    		return 0;
	
	switch (opt){
	    case kPut: {
	      std::string key = string(key_i);
	      std::string value = string(value_i);
	      db->Put(leveldb::WriteOptions(), key, value);
	      break;
	    }
	    case kGet: {
	      std::string key = string(key_i);
	      std::string value;
	      db->Get(leveldb::ReadOptions(), key, &value);
	      cout << value << endl;
	      break;
	    }
	    case kDelete: {
	      std::string key = string(key_i);
	      db->Delete(leveldb::WriteOptions(), key);
	      break;
	    }
	    case kGetProperty: {
	      std::string name = string(name_i);
	      std::string value;
	      db->GetProperty(name, &value);
	      break;
	    }
	    case kIterate: {
	      std::unique_ptr<leveldb::Iterator> it(
		  db->NewIterator(leveldb::ReadOptions()));
	      for (it->SeekToFirst(); it->Valid(); it->Next())
		continue;
	    }
	    case kGetReleaseSnapshot: {
	      leveldb::ReadOptions snapshot_options;
	      snapshot_options.snapshot = db->GetSnapshot();
	      std::unique_ptr<leveldb::Iterator> it(db->NewIterator(snapshot_options));
	      db->ReleaseSnapshot(snapshot_options.snapshot);
	    }
	    case kReopenDb: {
	      // The database must be closed before attempting to reopen it. Otherwise,
	      // the open will fail due to exclusive locking.
	      db.reset();
	      db = OpenDB();
	      if (!db)
		return 0;  // Reopening the database failed.
	      break;
	    }
	    case kCompactRange: {
	      std::string begin_key = string(begin_key_i);
	      std::string end_key = string(end_key_i);
	      leveldb::Slice begin_slice(begin_key);
	      leveldb::Slice end_slice(end_key);
	      db->CompactRange(&begin_slice, &end_slice);
	      break;
	    }	
	}
	
	return 0;
}
