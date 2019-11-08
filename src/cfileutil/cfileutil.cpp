/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#define SDL_MAIN_HANDLED

#include "SDL_endian.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <regex>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef PLAT_UNIX
#include <dirent.h>
#endif

// from cfile.h
#define CF_MAX_FILENAME_LENGTH 	32	// Includes null terminater, so real length is 31

extern void help();


class vp {
	public:
		enum action_t {
			INVALID = 0,
			LIST,
			CREATE,
			EXTRACT
		};

		vp();
		~vp();

		void setFilename(const char *val);
		void setOutputDirectory(const char *val);
		void setSourceDirectory(const char *val);
		void setLowerCase() { m_lower_case = true; }
		void setAction(action_t val) { m_action = val; }
		void setRegex(const char *val);

		action_t getAction() { return m_action; }

		void list();
		void create();
		void extract();

	private:
		typedef struct vp_header {
			int32_t id;				// 0x50565056 : 'VPVP' (little endian)
			int32_t version;
			int32_t index_offset;	// offset where the file index starts and total file data size
			int32_t num_files;		// number of files, including each step in directory structure
		} vp_header;

		typedef struct vp_fileindex {
			// part of VP
			int32_t offset;
			int32_t file_size;
			int32_t write_time;
			std::string file_name;

			// *not* part of VP, only used here
			std::string file_path;
		} vp_fileindex;

		const int32_t VP_VERSION = 2;
		const int32_t VP_ID = 0x50565056;
		const int32_t VP_HEADER_SIZE = 16;

		FILE *archive;

		void Seek(int32_t offset, int where);
		int32_t ReadInt();
		char *ReadString(char *buf, const size_t length);
		void WriteInt(const int32_t val);
		void WriteString(const std::string &buf, const size_t length);

		bool CreatePath(const std::string &path);

		void pack_directory(const std::string *pack_dir = nullptr);
		void add_directory(const std::string &a_dir);
		void add_file(const std::string &a_file, const long fsize, const time_t ftime);

		vp_header m_header;
		std::vector<vp_fileindex> m_index;

		void read_header();
		void read_index();

		void write_header();
		void write_index();

		std::string m_filename;
		std::string m_outdir;
		std::string m_sourcedir;
		bool m_lower_case;
		bool m_filtering;
		std::regex m_regex;

		action_t m_action;
};

void vp::Seek(int32_t offset, int where)
{
	int rval = fseek(archive, offset, where);

	if (rval) {
		if (errno == EINVAL) {
			throw std::runtime_error("invalid value in Seek()");
		} else {
			throw std::runtime_error("stream error in Seek()");
		}
	}
}

int32_t vp::ReadInt()
{
	int32_t result = -1;

	size_t rval = fread(&result, 1, sizeof(int32_t), archive);

	if (rval != sizeof(int32_t)) {
		throw std::runtime_error("short read in ReadInt()");
	}

	return SDL_SwapLE32(result);
}

char *vp::ReadString(char *buf, const size_t length)
{
	size_t rval = fread(buf, 1, length, archive);

	if (rval != length) {
		throw std::runtime_error("short read in ReadString()");
	}

	return buf;
}

void vp::WriteInt(const int32_t val)
{
	int32_t value = SDL_SwapLE32(val);

	size_t rval = fwrite(&value, 1, sizeof(int32_t), archive);

	if (rval != sizeof(int32_t)) {
		throw std::runtime_error("short write in WriteInt()");
	}
}

void vp::WriteString(const std::string &buf, const size_t length)
{
	if (buf.size() > length) {
		throw std::runtime_error("string length greater than write length in WriteString()");
	}

	size_t rval = fwrite(buf.c_str(), 1, buf.size(), archive);

	if (rval != buf.size()) {
		throw std::runtime_error("short write in WriteString()");
	}

	// we need to write the full 'length', so zero fill any extra space
	size_t fill = length - buf.size();

	if (fill) {
		const char zero = '\0';
		rval = 0;

		for (size_t idx = 0; idx < fill; idx++) {
			rval += fwrite(&zero, 1, 1, archive);
		}

		if (rval != fill) {
			throw std::runtime_error("short fill write in WriteString()");
		}
	}
}

bool vp::CreatePath(const std::string &path)
{
	std::string sub_path;
	std::string::size_type pos;

	pos = path.find('/');

	while (pos != std::string::npos) {
		sub_path = path.substr(0, pos);

		int status = mkdir(sub_path.c_str(), 0755);

		if (status && (errno != EEXIST)) {
			return false;
		}

		pos = path.find('/', pos+1);
	}

	return true;
}

void vp::read_header()
{
	if (archive == nullptr) {
		if ( m_filename.empty() ) {
			throw std::runtime_error("empty filename passed to vp::read_header()");
		}

		archive = fopen(m_filename.c_str(), "rb");

		if (archive == nullptr) {
			int err = errno;
			std::ostringstream errmsg;

			errmsg << "error opening '" << m_filename << "': " << std::strerror(err);
			throw std::runtime_error(errmsg.str());
		}
	}

	Seek(0, SEEK_SET);

	m_header.id = ReadInt();
	m_header.version = ReadInt();
	m_header.index_offset = ReadInt();
	m_header.num_files = ReadInt();

	if (m_header.id != vp::VP_ID) {
		throw std::runtime_error("invalid VP ID");
	}

	if (m_header.version != vp::VP_VERSION) {
		throw std::runtime_error("invalid VP version");
	}
}

void vp::write_header()
{
	if (archive == nullptr) {
		if ( m_filename.empty() ) {
			throw std::runtime_error("empty filename passed to vp::write_header()");
		}

		archive = fopen(m_filename.c_str(), "wb");

		if (archive == nullptr) {
			int err = errno;
			std::ostringstream errmsg;

			errmsg << "error opening '" << m_filename << "': " << std::strerror(err);
			throw std::runtime_error(errmsg.str());
		}
	}

	Seek(0, SEEK_SET);

	WriteInt(vp::VP_ID);
	WriteInt(vp::VP_VERSION);
	WriteInt(m_header.index_offset);
	WriteInt(m_header.num_files);
}

void vp::read_index()
{
	Seek(m_header.index_offset, SEEK_SET);

	vp_fileindex vpinfo;
	char filename_tmp[CF_MAX_FILENAME_LENGTH];
	std::string path;

	memset(filename_tmp, 0, sizeof(filename_tmp));

	// pre-allocate max size
	m_index.reserve(static_cast<size_t>(m_header.num_files));

	for (int i = 0; i < m_header.num_files; i++) {
		vpinfo.offset = ReadInt();
		vpinfo.file_size = ReadInt();
		vpinfo.file_name = ReadString(filename_tmp, CF_MAX_FILENAME_LENGTH);
		vpinfo.write_time = ReadInt();

		if (m_lower_case) {
			std::transform(vpinfo.file_name.begin(), vpinfo.file_name.end(),
						   vpinfo.file_name.begin(), ::tolower);
		}

		// check if it's a directory and if so then create a path to use for files
		if (vpinfo.file_size == 0) {
			// if we get a ".." then drop down in the path
			if ( !vpinfo.file_name.compare("..") ) {
				std::string::size_type pos = path.find_last_of('/');

				if (pos != std::string::npos) {
					path.erase(pos);
				}
			}
			// otherwise add it to the path
			else {
				if ( !path.empty() ) {
					path.push_back('/');
				}

				path.append(vpinfo.file_name);
			}
		}
		// otherwise it's a file
		else {
			vpinfo.file_path = path;

			m_index.push_back(vpinfo);
		}
	}
}

void vp::write_index()
{
	// index is appended to the end
	Seek(0, SEEK_END);

	std::vector<vp_fileindex>::iterator it;

	for (it = m_index.begin(); it != m_index.end(); ++it) {
		WriteInt(it->offset);
		WriteInt(it->file_size);
		WriteString(it->file_name, CF_MAX_FILENAME_LENGTH);
		WriteInt(it->write_time);
	}
}

void vp::pack_directory(const std::string *pack_dir)
{
	DIR *dirp;
	struct dirent *dir;
	struct stat buf;
	std::string source_path, path;

	if (pack_dir != nullptr) {
		source_path = *pack_dir;
	} else {
		source_path = m_sourcedir;
	}

	dirp = opendir(source_path.c_str());

	if (dirp == nullptr) {
		int err = errno;
		std::ostringstream errmsg;

		errmsg << "error opening path '" << source_path << "': " << std::strerror(err);
		throw std::runtime_error(errmsg.str());
	}

	std::cout << "  Scanning '" << source_path << "' ...\n";

	add_directory(source_path);

	while ( (dir = readdir(dirp)) != nullptr ) {
		std::string name = dir->d_name;

		if ( !name.compare(".") || !name.compare("..") ) {
			continue;
		}

		if (name.length() >= CF_MAX_FILENAME_LENGTH) {
			size_t half_len = std::min(name.length() / 2, static_cast<size_t>(12));
			std::string first_half = name.substr(0, half_len);
			std::string last_half = name.substr(name.length() - half_len);

			std::cout << "  Skipping '" << source_path << "/" << first_half
					  << "..." << last_half << "' ... Name too long (> "
					  << CF_MAX_FILENAME_LENGTH-1 << " characters)\n";
			continue;
		} else if (name.length() > 2) {
			std::string ext = name.substr(name.length()-3);
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if ( !ext.compare(".vp") ) {
				continue;
			}
		}

		path = source_path;

		path.push_back('/');
		path.append(dir->d_name);

		if ( stat(path.c_str(), &buf) == -1 ) {
			continue;
		}

		if (buf.st_size > INT32_MAX) {
			std::cout << "  Skipping '" << source_path << "/" << name
					  << "' ... Size is too large (> " << INT32_MAX << " bytes)\n";
			continue;
		}

		if ( S_ISDIR(buf.st_mode) ) {
			// recurse into new directory
			pack_directory(&path);
		} else {
			if (buf.st_size > 0) {
				add_file(path, buf.st_size, buf.st_mtime);
			}
		}
	}

	closedir(dirp);

	// add a final directory root
	add_directory(std::string(".."));
}

void vp::add_directory(const std::string &a_dir)
{
	vp_fileindex vpinfo;

	vpinfo.offset = static_cast<int32_t>(ftell(archive));
	vpinfo.file_size = 0;
	vpinfo.write_time = 0;
	vpinfo.file_name = a_dir;

	// stip extra path from directory name
	std::string::size_type pos = vpinfo.file_name.find_last_of('/');

	if (pos != std::string::npos) {
		vpinfo.file_name = vpinfo.file_name.substr(pos+1);
	}

	m_index.push_back(vpinfo);
}

void vp::add_file(const std::string &a_file, const long fsize, const time_t ftime)
{
	vp_fileindex vpinfo;

	// add file info to index...
	vpinfo.offset = static_cast<int32_t>(ftell(archive));
	vpinfo.file_size = static_cast<int32_t>(fsize);
	vpinfo.write_time = static_cast<int32_t>(ftime);
	vpinfo.file_name = a_file;

	// strip extra path from file name
	std::string::size_type pos = vpinfo.file_name.find_last_of('/');

	if (pos != std::string::npos) {
		vpinfo.file_name = vpinfo.file_name.substr(pos+1);
	}

	m_index.push_back(vpinfo);

	// add the file data to archive...
	FILE *infile = fopen(a_file.c_str(), "rb");

	if (infile == nullptr) {
		int err = errno;
		std::ostringstream errmsg;

		errmsg << "error opening input file '" << a_file << "': " << std::strerror(err);
		throw std::runtime_error(errmsg.str());
	}

	std::cout << "    Adding '" << a_file << "' ... ";

	const size_t BLOCK_SIZE = 1024*1024;

	char data_block[BLOCK_SIZE];
	size_t total_bytes = 0, num_bytes;

	do {
		num_bytes = fread(data_block, 1, BLOCK_SIZE, infile);

		if (num_bytes > 0) {
			fwrite(data_block, 1, num_bytes, archive);
			total_bytes += num_bytes;
		}
	} while (num_bytes > 0);

	fclose(infile);

	std::cout << total_bytes << " bytes\n";
}

vp::vp()
{
	archive = nullptr;
	m_lower_case = false;
	m_filtering = false;
	m_action = INVALID;

	m_header.index_offset = VP_HEADER_SIZE;	// size of vp_header (4 * sizeof(int32_t))
	m_header.num_files = 0;
}

vp::~vp()
{
	if (archive != nullptr) {
		fclose(archive);
		archive = nullptr;
	}
}

void vp::setFilename(const char *val)
{
	if ( !val ) {
		return;
	}

	m_filename = val;
}

void vp::setOutputDirectory(const char *val)
{
	if ( !val ) {
		return;
	}

	m_outdir = val;

	if (m_outdir.back() == '/') {
		m_outdir.pop_back();
	}
}

void vp::setSourceDirectory(const char *val)
{
	if ( !val ) {
		return;
	}

	m_sourcedir = val;

	if (m_sourcedir.back() == '/') {
		m_sourcedir.pop_back();
	}

	// make sure that it's named 'data', case insensitive
	std::string is_data;
	std::string::size_type pos = m_sourcedir.find_last_of('/');

	if (pos != std::string::npos) {
		is_data = m_sourcedir.substr(pos+1);
	} else {
		is_data = m_sourcedir;
	}

	std::transform(is_data.begin(), is_data.end(), is_data.begin(), ::tolower);

	if (is_data.compare("data") ) {
		throw std::runtime_error("source directory must be named 'data'");
	}
}

void vp::setRegex(const char *val)
{
	if ( !val ) {
		return;
	}

	try {
		m_regex = val;
	} catch (const std::regex_error &e) {
		std::cout << "regex validation failure: " << e.what() << std::endl;
	}

	m_filtering = true;
}

void vp::list()
{
	if (getAction() != vp::LIST) {
		return;
	}

	read_header();
	read_index();

	std::cout << "VP file archiver/extractor - version 1.0\n";
	std::cout << std::endl;
	std::cout << m_filename << " ...\n";
	std::cout << std::endl;

	std::cout << "   Size       Date      Time    Name\n";
	std::cout << "---------- ---------- --------  ------------------------\n";

	//char time_str[30];
	time_t write_time;
	std::vector<vp_fileindex>::iterator it;

	for (it = m_index.begin(); it != m_index.end(); ++it) {
		write_time = it->write_time;

		std::cout << std::setw(10) << it->file_size << " "
				  << std::put_time(std::gmtime(&write_time), "%F %H:%M:%S")
				  << "  " << it->file_path << "/" << it->file_name << std::endl;
	}

	std::cout << "---------- ---------- --------  ------------------------\n";
	std::cout << std::setw(10) << m_header.index_offset-VP_HEADER_SIZE
			  << "                      " << m_index.size() << " file(s)\n";

	std::cout << std::endl;
}

void vp::create()
{
	if (getAction() != vp::CREATE) {
		return;
	}

	std::string ext;

	if (m_filename.length() > 3) {
		ext = m_filename.substr(m_filename.length()-3);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	}

	if ( ext.compare(".vp") ) {
		m_filename.append(".vp");
	}

	std::cout << "VP file archiver/extractor - version 1.0\n";
	std::cout << std::endl;
	std::cout << "Creating archive '" << m_filename << "' ...\n";
	std::cout << std::endl;

	// TODO: verify directory

	// write header place-holder
	write_header();

	// do files
	pack_directory();

	// update header with proper values
	Seek(0, SEEK_END);

	m_header.index_offset = static_cast<int32_t>(ftell(archive));
	m_header.num_files = static_cast<int32_t>(m_index.size());

	write_header();

	// finalize with file index and summary
	write_index();

	size_t total_files = 0;
	std::vector<vp_fileindex>::iterator it;

	for (it = m_index.begin(); it != m_index.end(); ++it) {
		if (it->file_size > 0) {
			++total_files;
		}
	}

	Seek(0, SEEK_END);

	std::cout << std::endl;
	std::cout << "  Added " << total_files << " files\n";
	std::cout << "  Total size: " << ftell(archive) << " bytes\n";
	std::cout << std::endl;
}

void vp::extract()
{
	if (getAction() != vp::EXTRACT) {
		return;
	}

	read_header();
	read_index();

	std::cout << "VP file archiver/extractor - version 1.0\n";
	std::cout << std::endl;

	if ( !m_outdir.empty() ) {
		std::cout << "Output directory: " << m_outdir << std::endl << std::endl;
	}

	std::cout << "Extracting '" << m_filename << "' ...\n";

	const size_t BLOCK_SIZE = 1024*1024;

	std::vector<vp_fileindex>::iterator it;
	std::string path, rel_path;
	FILE *outfile;
	int32_t bytes_remaining;
	size_t rval;
	char data_block[BLOCK_SIZE];
	size_t ex_count = 0;
	std::smatch match;

	for (it = m_index.begin(); it != m_index.end(); ++it) {
		rel_path = it->file_path;

		rel_path.push_back('/');
		rel_path.append(it->file_name);

		if ( m_filtering && !std::regex_search(rel_path, match, m_regex) ) {
			continue;
		}

		std::cout << "  " << rel_path << " ... ";

		path = m_outdir;

		if ( !m_outdir.empty() ) {
			path.push_back('/');
		}

		path.append(rel_path);

		if ( !CreatePath(path) ) {
			std::cout << "ERROR: cannot create path!\n";
			continue;
		}

		outfile = fopen(path.c_str(), "wb");

		if (outfile == nullptr) {
			std::cout << "ERROR: cannot create file!\n";
			continue;
		}

		Seek(it->offset, SEEK_SET);

		bytes_remaining = it->file_size;

		while (bytes_remaining > 0) {
			rval = fread(data_block, 1, std::min(BLOCK_SIZE, static_cast<size_t>(bytes_remaining)), archive);

			if (rval > 0) {
				fwrite(data_block, 1, rval, outfile);
				bytes_remaining -= rval;
			}
		}

		fclose(outfile);

		++ex_count;

		std::cout << "done!\n";
	}

	std::cout << std::endl;
	std::cout << "  " << ex_count << (m_filtering ? " matching" : "") << " files extracted\n";
	std::cout << std::endl;
}

static vp VP;


void help()
{
	std::cout << "VP file archiver/extractor - version 1.0\n";
	std::cout << std::endl;
	std::cout << "Usage: cfileutil c <vp_filename> <source_dir>\n";
	std::cout << "       cfileutil x [-L] [-o <dir>] [-f <regex>] <vp_filename>\n";
	std::cout << "       cfileutil l <vp_filename>\n";
	std::cout << std::endl;
	std::cout << " Commands:\n";
	std::cout << "  c           Create VP archive from <source_dir>\n";
	std::cout << "  x           Extract all files into current directory\n";
	std::cout << "  l           List all files in VP archive\n";
	std::cout << std::endl;
	std::cout << " Extraction options:\n";
	std::cout << "  -L          Force all directory and file names to be lower case\n";
	std::cout << "  -o <dir>    Extract into <dir> rather than current directory\n";
	std::cout << "  -f <regex>  Only extract files matching regex\n";
	std::cout << std::endl;

	exit(EXIT_SUCCESS);
}

#define MAYBE_HELP(x) if (argc - idx < (x)) { help(); }

void parse_args(int argc, char *argv[])
{
	for (int idx = 1; idx < argc; idx++) {
		if ( !std::strcmp(argv[idx], "-h") || !std::strcmp(argv[idx], "--help") ) {
			help();
		} else if ( !std::strcmp(argv[idx], "c") ) {
			idx++;

			MAYBE_HELP(2);

			if (VP.getAction() != vp::INVALID) {
				help();
			}

			VP.setFilename(argv[idx++]);
			VP.setSourceDirectory(argv[idx]);
			VP.setAction(vp::CREATE);
		} else if ( !std::strcmp(argv[idx], "l") ) {
			idx++;

			MAYBE_HELP(1);

			if (VP.getAction() != vp::INVALID) {
				help();
			}

			VP.setFilename(argv[idx]);
			VP.setAction(vp::LIST);
		} else if ( !std::strcmp(argv[idx], "x") ) {
			idx++;

			MAYBE_HELP(1);

			if (VP.getAction() != vp::INVALID) {
				help();
			}

			for ( ; idx < argc; idx++) {
				if ( !std::strcmp(argv[idx], "-L") ) {
					MAYBE_HELP(1);

					VP.setLowerCase();
				} else if ( !std::strcmp(argv[idx], "-o") ) {
					idx++;

					MAYBE_HELP(1);

					VP.setOutputDirectory(argv[idx]);
				} else if ( !std::strcmp(argv[idx], "-f") ) {
					idx++;

					MAYBE_HELP(1);

					VP.setRegex(argv[idx]);
				} else {
					VP.setFilename(argv[idx]);
					VP.setAction(vp::EXTRACT);
				}
			}
		} else {
			help();
		}
	}
}

extern "C"
int main(int argc, char *argv[])
{
	if (argc < 3) {
		help();
	}

	try {
		parse_args(argc, argv);
	} catch (const std::regex_error &e) {
		std::cerr << "Error validating regex => " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (const std::exception &e) {
		std::cerr << "Error parsing arguments => " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	switch ( VP.getAction() ) {
		case vp::LIST: {
			try {
				VP.list();
			} catch (const std::exception &e) {
				std::cerr << "Error listing archive => " << e.what() << std::endl;
				return EXIT_FAILURE;
			}

			break;
		}

		case vp::CREATE: {
			try {
				VP.create();
			} catch (const std::exception &e) {
				std::cerr << "Error creating archive => " << e.what() << std::endl;
				return EXIT_FAILURE;
			}

			break;
		}

		case vp::EXTRACT: {
			try {
				VP.extract();
			} catch(const std::exception &e) {
				std::cerr << "Error extracting archive => " << e.what() << std::endl;
				return EXIT_FAILURE;
			}

			break;
		}

		default:
			help();
			break;
	}

	return EXIT_SUCCESS;
}
