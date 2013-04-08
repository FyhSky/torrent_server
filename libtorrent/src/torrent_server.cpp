#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/progress.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>

#include "libtorrent/extensions/logger.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/string_util.hpp"

#include "libtorrent/escape_string.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/extern_read_op.hpp"

#include "libtorrent/torrent_server.hpp"
#include "libtorrent/http_server.hpp"

// using namespace libtorrent;
#if BOOST_VERSION < 103400
namespace fs = boost::filesystem;
fs::path::default_name_check(fs::no_check);
#endif

namespace libtorrent {

torrent_server::torrent_server(libtorrent::session &ses)
	: m_ses(ses)
{}

torrent_server::~torrent_server(void)
{}

bool torrent_server::create_server(short port)
{
	// �õ�ses�е�����torrent_handle, �õ�������Ƶ�ļ���Ϣ.
	int index = 0;

	boost::mutex::scoped_lock l(m_mutex);

	m_torrents = m_ses.get_torrents();
	for (std::vector<torrent_handle>::iterator
		i = m_torrents.begin(); i != m_torrents.end(); i++)
	{
		// ����Ϊ���ǵ�����ģʽ.
		i->set_user_defined_download(true);

		// ������չAPI�ӿ�.
		boost::shared_ptr<extern_read_op> read_op(new extern_read_op(*i, m_ses));

		// �õ�torrent_info.
		const torrent_info &info = i->get_torrent_info();

		// ������Ƶ�ļ�.
		const file_storage &fs = info.files();
		for (file_storage::iterator f = fs.begin();
			f != fs.end(); f++)
		{
			boost::filesystem::path p(convert_to_native(f->filename()));
			std::string ext = p.extension().string();
			if (ext == ".rmvb" ||
				ext == ".wmv" ||
				ext == ".avi" ||
				ext == ".mkv" ||
				ext == ".flv" ||
				ext == ".rm" ||
				ext == ".mp4" ||
				ext == ".3gp" ||
				ext == ".webm" ||
				ext == ".mpg")
			{
				video_info vi;

				char filename[1024] = {0};
				strcpy(filename, p.string().c_str());
				vi.file_name = filename;
				vi.base_offset = f->offset;
				vi.file_size = f->size;
				vi.index = index;
				vi.read_op = read_op;

				// ���浽����.
				m_video_infos.push_back(vi);

				index++;
			}
		} // end for.

		if (index == 0) return false;	// һ����Ƶ�ļ�Ҳû��.

		if (!m_server)
		{
			std::ostringstream os;
			os << port;
			std::string port_string = os.str();
			// ����������.
			std::string address = "0.0.0.0";
			m_server.reset(new http::server::server(address, port_string,
				".", boost::bind(&torrent_server::request_handle, this, _1, _2, _3),
				boost::bind(&torrent_server::read_handle, this, _1, _2, _3, _4)
			));

			// ����io_service.run�߳�.
			m_server_thread = boost::thread(boost::bind(&http::server::server::run, m_server.get()));
		}
	}
}

void torrent_server::destory_server()
{
	m_server->stop();
	m_server_thread.join();
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_video_infos.clear();
	}
}

bool torrent_server::request_handle(const std::string &uri, boost::int64_t &file_size, int &index)
{
	// ��֤uri.
	boost::filesystem::path path(uri);
	std::string file_name = path.leaf().string();

	boost::mutex::scoped_lock l(m_mutex);

	// ����BT�ж�Ӧ���ļ�.
	for (std::vector<video_info>::iterator i = m_video_infos.begin(); i != m_video_infos.end(); i++)
	{
		if (i->file_name == file_name)
		{
			index = i->index;
			file_size = i->file_size;
			return true;
		}
	}

	return false;
}

bool torrent_server::read_handle(int index, boost::int64_t offset, char *buffer, int &read_size)
{
	std::vector<video_info>::iterator finder;
	{
		boost::mutex::scoped_lock l(m_mutex);
		// ����BT�ж�Ӧ���ļ�.
		for (finder = m_video_infos.begin(); finder != m_video_infos.end(); finder++)
		{
			if (finder->index == index)
				break;
		}
		if (finder == m_video_infos.end())
			return false;
	}

	size_type ret = 0;
	// TODO: ����session��ǰ�˳�, read_op��ȡ���ݵ�����.
	finder->read_op->read_data(buffer, offset, read_size, ret);
	read_size = ret;

	return true;
}

}
