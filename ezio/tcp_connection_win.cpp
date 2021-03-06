/*
 @ 0xCCCCCCCC
*/

#include "ezio/tcp_connection.h"

#include <WinSock2.h>

#include "kbase/error_exception_util.h"

#include "ezio/event_loop.h"

namespace ezio {

void TCPConnection::PostRead()
{
    ENSURE(CHECK, loop_->BelongsToCurrentThread()).Require();
    ENSURE(CHECK, conn_notifier_.WatchReading())(conn_notifier_.watching_events()).Require();

    DWORD flags = 0;
    WSABUF buf {};

    // Provide buffer to receive data if we are not probing.
    if (!io_reqs_.read_req.IsProbing()) {
        buf.buf = input_buf_.BeginWrite();
        buf.len = static_cast<ULONG>(input_buf_.writable_size());
    }

    io_reqs_.read_req.Reset();

    int rv = WSARecv(conn_sock_.get(), &buf, 1, nullptr, &flags, &io_reqs_.read_req, nullptr);
    if (rv != 0 && WSAGetLastError() != WSA_IO_PENDING) {
        HandleError();
    }
}

void TCPConnection::HandleRead(TimePoint timestamp, IOContext::Details details)
{
    if ((details.events & IOEvent::Probe) != 0) {
        io_reqs_.read_req.DisableProbing();
    } else {
        if (details.bytes_transferred == 0) {
            HandleClose();
            return;
        }

        input_buf_.EndWrite(details.bytes_transferred);

        on_message_(shared_from_this(), input_buf_, timestamp);
    }

    PostRead();
}

void TCPConnection::DoSend(kbase::StringView data)
{
    ENSURE(CHECK, loop_->BelongsToCurrentThread()).Require();

    output_buf_.Write(data.data(), data.size());

    if (!io_reqs_.outstanding_write_req) {
        // The last PostWrite() may fail and we are still watching writing.
        if (!conn_notifier_.WatchWriting()) {
            conn_notifier_.EnableWriting();
        }

        PostWrite();
    }
}

void TCPConnection::PostWrite()
{
    ENSURE(CHECK, loop_->BelongsToCurrentThread()).Require();
    ENSURE(CHECK, conn_notifier_.WatchWriting())(conn_notifier_.watching_events()).Require();
    ENSURE(CHECK, !io_reqs_.outstanding_write_req).Require();

    DWORD flags = 0;

    WSABUF buf {};
    buf.buf = const_cast<char*>(output_buf_.Peek());
    buf.len = static_cast<ULONG>(output_buf_.readable_size());

    io_reqs_.write_req.Reset();

    int rv = WSASend(conn_sock_.get(), &buf, 1, nullptr, flags, &io_reqs_.write_req, nullptr);
    if (rv != 0 && WSAGetLastError() != WSA_IO_PENDING) {
        HandleError();
        return;
    }

    io_reqs_.outstanding_write_req = true;
}

void TCPConnection::HandleWrite(IOContext::Details details)
{
    ENSURE(CHECK, loop_->BelongsToCurrentThread()).Require();
    ENSURE(CHECK, io_reqs_.outstanding_write_req).Require();

    io_reqs_.outstanding_write_req = false;

    output_buf_.Consume(details.bytes_transferred);

    if (output_buf_.readable_size() > 0) {
        PostWrite();
    } else {
        conn_notifier_.DisableWriting();
        if (state() == State::Disconnecting) {
            DoShutdown();
        }
    }
}

}   // namespace ezio
