//
// torrent_server.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Jack (jack.wgm@gmail.com)
//
// This file is part of Libavplayer.
//
// Libavplayer is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// Libavplayer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with Libavplayer; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// * $Id: torrent_server.h 98 2011-08-19 16:08:52Z jack $
//

#ifndef __TORRENT_SERVER_H__
#define __TORRENT_SERVER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

// ����libtorrent�е�ʵ��.
namespace libtorrent {

	class session;
	struct torrent_handle;
	class extern_read_op;

	// ����http server��ʵ��.
	namespace http {
		namespace server {
			class server;
		}
	}
}


namespace libtorrent {

// ��Ƶ��Ϣ.
struct video_info
{
	std::string file_name;			// �ļ���.
	std::string hash;				// hash.
	int index;						// ��libtorrent�е�index.
	boost::int64_t base_offset;		// ��libtorrent�е�ƫ��.
	boost::int64_t file_size;		// �ļ���С.
	std::string url;				// �ⲿ���ʵ�url.
	// ����API.
	boost::shared_ptr<extern_read_op> read_op;
};

// �����serverʵ��.
// ʹ�÷���, ʾ������
// ����:
//  libtorrent::session s;
//  ...
//  torrent_server serv(s);
//  // ��������8889�˿�.
//  serv.create_server(8889);
//  ...
// ����:
//  serv.destory_server();
//  s.stop();
// ע��: ����������libtorrent::session, torrent_server��˳����Ū����.
class torrent_server : boost::noncopyable
{
public:
	torrent_server(session &ses);
	~torrent_server(void);

	// ����http�ķ������˿�.
	bool create_server(short port);

	// ����Http����.
	void destory_server();

	// �����е���Ƶ�б�.
	std::vector<video_info> video_list();

protected:
	bool request_handle(const std::string &uri, boost::int64_t &file_size, int &index);
	bool read_handle(int index, boost::int64_t offset, char *buffer, int &read_size);

private:
	libtorrent::session &m_ses;
	boost::mutex m_mutex;
	std::vector<video_info> m_video_infos;
	std::vector<torrent_handle> m_torrents;
	boost::shared_ptr<http::server::server> m_server;
	boost::thread m_server_thread;
};

}

#endif // __TORRENT_SERVER_H__
