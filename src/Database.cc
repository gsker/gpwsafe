//
// Database.cc
//
//     Created: 21.01.2012
//      Author: A. Sakhnik
//
// This file is part of gpwsafe.
//
// Gpwsafe is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Gpwsafe is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gpwsafe.  If not, see <http://www.gnu.org/licenses/>

#include "Database.hh"
#include "Memory.hh"
#include "Debug.hh"
#include "i18n.h"

#include <iostream>
#include <errno.h>
#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace gPWS {

using namespace std;

void *cDatabase::operator new(size_t n)
{
	return SecureAllocator<cDatabase>::allocate(n);
}

void cDatabase::operator delete(void *p, size_t n)
{
	cDatabase *q = reinterpret_cast<cDatabase *>(p);
	return SecureAllocator<cDatabase>::deallocate(q, n);
}

cDatabase::cDatabase()
	: _changed(false)
{
	_fields.reserve(0x0F);
}

void cDatabase::Create()
{
	_fields.clear();
	_changed = false;

	sField::PtrT field(sField::Create());

	// Insert default version (mock 3.10)
	field->type = FT_VERSION;
	field->value.clear();
	field->value.push_back(0x0A);
	field->value.push_back(0x03);
	_AddField(field);

	// Insert new UUID
	field->type = FT_UUID;
	field->value.resize(16);
	boost::uuids::uuid new_uuid = boost::uuids::random_generator()();
	assert(new_uuid.size() == 16);
	copy(new_uuid.begin(), new_uuid.end(), field->value.begin());
	_AddField(field);
}

void cDatabase::Read(string const &fname,
                     StringX const &pass)
{
	// The database must be clear.
	assert(!_changed && "The changes must be written beforehand");

	_changed = false;
	_fname = fname;
	_pass = pass;
	_file.OpenRead(fname.c_str(), pass);

	// Read header fields first
	sField::PtrT field;
	while ((field = _file.ReadField()))
	{
		if (!_AddField(field))
			break;
	}

	if (!field)
		return;

	cEntry::PtrT entry(cEntry::Create());
	while ((field = _file.ReadField()))
	{
		if (!entry->AddField(field))
		{
			// FIXME: Check if such key already exists.
			// Avoid loosing information.
			_entries[entry->GetFullTitle()] = entry;
			entry = cEntry::Create();
		}
	}
}

void cDatabase::Write(string const &fname,
                      StringX const &pass)
{
	_changed = false;
	_file.OpenWrite(fname.c_str(), pass, true);

	for (auto &field : _fields)
	{
		if (field)
			_file.WriteField(field);
	}

	// Field terminator
	sField::PtrT terminator(sField::Create());
	terminator->type = 0xFF;
	terminator->value.clear();
	_file.WriteField(terminator);

	for (auto &title_entry : _entries)
	{
		auto &entry = title_entry.second;
		entry->ForEachField([this](sField::PtrT const &field) {
							_file.WriteField(field);
							});
		_file.WriteField(terminator);
	}

	_file.CloseWrite();
}

string cDatabase::FollowSymlink(const string &fname)
{
	char buf[PATH_MAX + 1];
	memcpy(buf, fname.data(), fname.size());
	buf[fname.size()] = 0;

	ssize_t len = 0;
	while (-1 != (len = readlink(buf, buf, sizeof(buf) - 1)))
		buf[len] = 0;
	if (errno != EINVAL)
	{
		auto e = errno;
		cerr << _("Couldn't follow a symbolic link ") << buf << ": "
			<< strerror(e) << endl;
	}

	return buf;
}

void cDatabase::Write()
{
	assert(!_fname.empty() && !_pass.empty());

	auto fname = FollowSymlink(_fname);
	string new_fname = fname + ".new";
	string backup = fname + "~";

	::unlink(new_fname.c_str());
	Write(new_fname, _pass);

	// The following calls are unlikely to fail.
	::unlink(backup.c_str());

	if (-1 == ::rename(fname.c_str(), backup.c_str()))
	{
		cerr << _("Failed to create backup: ") << strerror(errno) << endl;
		throw runtime_error("File system");
	}

	if (-1 == ::rename(new_fname.c_str(), fname.c_str()))
	{
		cerr << _("Failed to move new file: ") << strerror(errno) << endl;
		throw runtime_error("File system");
	}
}

bool cDatabase::_AddField(sField::PtrT const &field)
{
	if (field->type == 0xFF)
		return false;
	if (_fields.size() <= field->type)
		_fields.resize(static_cast<unsigned>(field->type) + 1);
	_fields[field->type] = field;
	return true;
}

void cDatabase::Dump() const
{
	for (auto const &field : _fields)
	{
		if (!field)
			continue;
		cout << "{0x"
		     << boost::format("%02X") % unsigned(field->type)
		     << ", '"
		     << gPWS::Quote(&field->value[0], field->value.size())
		     << "'}"
		     << endl;
	}
	cout << "==========" << endl;

	for (auto const &keyval : _entries)
	{
		auto const &entry = keyval.second;
		entry->Dump();
	}
}

void cDatabase::AddEntry(cEntry::PtrT const &entry)
{
	// FIXME: Check if such key already exists. Avoid loosing information.
	_entries[entry->GetFullTitle()] = entry;
	_changed = true;
}

void cDatabase::RemoveEntry(cEntry::PtrT const &entry)
{
	_entries.erase(entry->GetFullTitle());
	_changed = true;
}

bool cDatabase::HasEntry(StringX const &full_title) const
{
	return _entries.find(full_title) != _entries.end();
}

cDatabase::MatchT cDatabase::Find(char const *query) const
{
	MatchT match;
	for (auto first = begin(_entries), last = end(_entries);
	     first != last;
	     ++first)
	{
		if (!query || first->first.find(query) != first->first.npos)
			match.push_back(first);
	}
	return match;
}

} //namespace gPWS;

// vim: set noet ts=4 sw=4 tw=80:
