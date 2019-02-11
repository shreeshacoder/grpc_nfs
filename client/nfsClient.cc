#include "nfsClient.h"

#define LOG true

clientImplementation::clientImplementation(std::shared_ptr<Channel> channel)
	: stub_(NfsServer::NewStub(channel)) {}

static std::map<int, Datastore> mainDataStore;

static int buffer_size = 65536;

int clientImplementation::client_mkdir(std::string path, mode_t mode)
{

	mkdir_request request;
	c_response response;
	attributes atr;
	ClientContext context;

	request.set_dirfh(path);
	atr.set_st_mode(mode);
	request.mutable_attr()->CopyFrom(atr);

	std::cout << "calling mkdir: " << std::endl;
	Status status = stub_->server_mkdir(&context, request, &response);

	if (status.ok())
	{
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_rmdir(std::string path)
{

	rmdir_request request;
	c_response response;
	ClientContext context;

	request.set_dirfh(path);

	std::cout << "calling rmdir: " << std::endl;
	Status status = stub_->server_rmdir(&context, request, &response);

	if (status.ok())
	{
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_rename(std::string from, std::string to)
{

	rename_request request;
	c_response response;
	ClientContext context;

	request.set_fromdir(from);
	request.set_todir(to);

	std::cout << "calling rename: " << std::endl;
	Status status = stub_->server_rename(&context, request, &response);

	if (status.ok())
	{
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_open(std::string path, struct fuse_file_info *fi)
{

	open_request request;
	d_response response;
	ClientContext context;

	request.set_fh(path);
	request.mutable_pfi()->CopyFrom(toProtoFileInfo(fi));

	std::cout << "calling open: " << std::endl;
	Status status = stub_->server_open(&context, request, &response);

	if (status.ok())
	{
		toFuseFileInfo(response.pfi(), fi);
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_create(std::string path, mode_t mode, struct fuse_file_info *fi)
{

	create_truncate_request request;
	d_response response;
	attributes atr;
	ClientContext context;

	request.set_fh(path);

	atr.set_st_mode(mode);
	request.mutable_attr()->CopyFrom(atr);
	request.mutable_pfi()->CopyFrom(toProtoFileInfo(fi));

	std::cout << "calling create: " << std::endl;
	Status status = stub_->server_create(&context, request, &response);

	if (status.ok())
	{
		toFuseFileInfo(response.pfi(), fi);
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_truncate(std::string path, off_t size, struct fuse_file_info *fi)
{

	create_truncate_request request;
	attributes atr;
	d_response response;
	ClientContext context;

	request.set_fh(path);
	atr.set_st_size(size);
	request.mutable_attr()->CopyFrom(atr);
	request.mutable_pfi()->CopyFrom(toProtoFileInfo(fi));

	std::cout << "calling truncate: " << std::endl;
	Status status = stub_->server_truncate(&context, request, &response);

	if (status.ok())
	{
		toFuseFileInfo(response.pfi(), fi);
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::client_unlink(std::string path)
{

	unlink_request request;
	c_response response;
	ClientContext context;

	request.set_fh(path);

	std::cout << "calling unlink: " << std::endl;
	Status status = stub_->server_unlink(&context, request, &response);

	if (status.ok())
	{
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
}

std::list<DirEntry> clientImplementation::read_directory(std::string path, int &responseCode)
{
	readdir_request request;
	readdir_response response;
	ClientContext context;

	request.set_path(path);

	std::cout << "calling readdir: " << std::endl;
	Status status = stub_->read_directory(&context, request, &response);

	std::list<DirEntry> entries;
	if (status.ok())
	{
		responseCode = response.status();
		for (int i = 0; i < response.objects_size(); i++)
		{
			DirEntry dirEntry;
			toCstat(response.objects(i).attr(), &dirEntry.st);
			dirEntry.name = response.objects(i).name();
			entries.push_back(dirEntry);
		}
		return entries;
	}
	else
	{
		return entries;
	}
}

int clientImplementation::get_attributes(std::string path, struct stat *st)
{

	// if (LOG)
	// 	std::cout << "------------------------------------------------\n";
	// if (LOG)
	// 	std::cout << "getAttributes : path passed - " << path << "\n";
	// Container request
	attribute_request_object getAttributesRequestObject;
	getAttributesRequestObject.set_path(path);
	*getAttributesRequestObject.mutable_attr() = toGstat(st);
	ClientContext context;

	// Container response
	attribute_response_object getAttributesResponseObject;
	// if (LOG)
	// 	std::cout << "getAttributes : Calling server \n";
	// Actual call
	Status status = stub_->get_attributes(&context, getAttributesRequestObject, &getAttributesResponseObject);
	// if (LOG)
	// 	std::cout << "getAttributes : Response from server \n";
	if (status.ok())
	{
		// if (LOG)
		// 	std::cout << "getAttributes : converting response \n";
		toCstat(getAttributesResponseObject.attr(), st);
		// if (LOG)
		// 	std::cout << "getAttributes : returning resposne \n";
		return getAttributesResponseObject.status();
	}
	else
	{
		// if (LOG)
		// 	std::cout << "getAttributes : Failed \n";
		// if (LOG)
		// 	std::cout << status.error_code() << ": " << status.error_message()
		// 			  << std::endl;

		// if (LOG)
		// 	std::cout << "------------------------------------------------\n\n";
		return -1;
	}
}

int clientImplementation::client_read(std::string path, char *buffer, int size, int offset, struct fuse_file_info *fi)
{
	read_request request;
	read_response response;
	ClientContext context;

	std::cout << "reading: path, " << path << " size, " << size << " offset, " << offset << "\n";

	request.set_path(path);
	request.set_offset(offset);
	request.set_size(size);
	request.mutable_pfi()->CopyFrom(toProtoFileInfo(fi));

	std::cout << "calling readdir: " << std::endl;
	Status status = stub_->server_read(&context, request, &response);

	toFuseFileInfo(response.pfi(), fi);
	if (status.ok())
	{
		strncpy(buffer, response.data().c_str(), size);
		return response.size();
	}
	else
	{
		return -1;
	}
}

int clientImplementation::client_mknod(std::string path, mode_t mode, dev_t rdev)
{

	read_directory_single_object request;
	attributes atr;
	c_response response;
	ClientContext context;

	request.set_name(path);
	atr.set_st_mode(mode);
	atr.set_st_dev(rdev);
	request.mutable_attr()->CopyFrom(atr);

	std::cout << "calling mknod: " << std::endl;
	Status status = stub_->server_mknod(&context, request, &response);

	if (status.ok())
	{
		if (response.success() != 0)
			return (-response.ern());
		return 0;
	}
	else
	{
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return -1;
	}
}

int clientImplementation::write(std::string path, const char *buf, int size, int offset, struct fuse_file_info *fi)
{

	write_request_object writeRequestObject;

	bool commitFlag = false;
	bool sendAll = false;
	Datastore sendAllDataStore;
	if (fi->fh != 0)
	{
		if (mainDataStore.find(fi->fh) != mainDataStore.end())
		{
			Datastore ds = mainDataStore.find(fi->fh)->second;

			if (ds.getIsDirty())
			{
				sendAll = true;
				commitFlag = true;
				sendAllDataStore = ds;
			}

			if (ds.getData().length() > buffer_size)
			{
				commitFlag = true;
			}
		}
	}

	writeRequestObject.set_flag(commitFlag);
	writeRequestObject.set_path(path);
	if (sendAll == false)
	{
		writeRequestObject.set_data(buf);
		writeRequestObject.set_offset(offset);
		writeRequestObject.set_size(size);
	}
	else
	{
		std::string temp(buf);
		std::string allDataToBeSent = sendAllDataStore.getData() + temp;
		char *newBuf = new char[allDataToBeSent.length() + 1];
		strncpy(newBuf, allDataToBeSent.c_str(), allDataToBeSent.length());
		writeRequestObject.set_data(newBuf);
		writeRequestObject.set_offset(sendAllDataStore.getOriginalOffset());
		writeRequestObject.set_size(allDataToBeSent.length());
	}
	*writeRequestObject.mutable_fileinfo() = toGFileInfo(fi);

	ClientContext context;

	// Container response
	write_response_object writeResponseObject;

	// Call
	Status status = stub_->write(&context, writeRequestObject, &writeResponseObject);

	if (status.ok())
	{
		toCFileInfo(writeResponseObject.fileinfo(), fi);
		if (commitFlag)
		{
			if (mainDataStore.find(fi->fh) != mainDataStore.end())
			{
				Datastore ds = mainDataStore.find(fi->fh)->second;
				ds.setValues("", 0, false);
				mainDataStore.find(fi->fh)->second = ds;
			}
		}
		else
		{
			std::string temp2(buf);
			if (mainDataStore.find(fi->fh) != mainDataStore.end())
			{
				Datastore ds = mainDataStore.find(fi->fh)->second;
				ds.setValues((ds.getData() + temp2), ds.getOriginalOffset());
				mainDataStore.find(fi->fh)->second = ds;
			}
			else
			{
				mainDataStore.insert(std::make_pair<int, Datastore>(fi->fh, Datastore(temp2, offset, false)));
			}
		}
		return writeResponseObject.datasize();
	}
	else
	{
		if (mainDataStore.find(fi->fh) != mainDataStore.end())
		{
			std::string temp2(buf);
			Datastore ds = mainDataStore.find(fi->fh)->second;
			ds.setValues((ds.getData() + temp2), ds.getOriginalOffset());
			ds.setDirty();
			mainDataStore.find(fi->fh)->second = ds;
			return size;
		}
		else
		{
			return -1;
		}
	}
	if (LOG)
		std::cout << "------------------------------------------------\n\n";
}

int clientImplementation::fsync(std::string path, int isdatasync, struct fuse_file_info *fi)
{
	fsync_request fsyncRequestObject;
	fsyncRequestObject.set_path(path);
	fsyncRequestObject.set_isdatasync(isdatasync);
	*fsyncRequestObject.mutable_fileinfo() = toGFileInfo(fi);
	ClientContext context;

	// Container response
	fsync_response fsyncResponseObject;

	// Call
	Status status = stub_->fsync(&context, fsyncRequestObject, &fsyncResponseObject);

	toCFileInfo(fsyncResponseObject.fileinfo(), fi);

	if (status.ok())
	{
		return fsyncResponseObject.status();
	}
	else
	{
		if (LOG)
			std::cout << status.error_code() << ": " << status.error_message()
					  << std::endl;
		return -1;
	}
}

int clientImplementation::flush(std::string path, struct fuse_file_info *fi)
{
	flush_request flushRequestObject;
	flushRequestObject.set_path(path);
	*flushRequestObject.mutable_fileinfo() = toGFileInfo(fi);
	ClientContext context;

	// Container response
	flush_response flushResponseObject;

	// Call
	Status status = stub_->flush(&context, flushRequestObject, &flushResponseObject);

	toCFileInfo(flushResponseObject.fileinfo(), fi);

	if (status.ok())
	{
		return flushResponseObject.status();
	}
	else
	{
		if (LOG)
			std::cout << status.error_code() << ": " << status.error_message()
					  << std::endl;
		return -1;
	}
}