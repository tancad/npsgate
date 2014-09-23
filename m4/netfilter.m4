AC_DEFUN([AX_NETFILTER_QUEUE], [
	AC_CHECK_HEADERS(libnetfilter_queue/libnetfilter_queue.h, [], [
		AC_MSG_FAILURE('Failed to find libnetfilter-queue headers.')])

	AC_CHECK_HEADERS(libnfnetlink/libnfnetlink.h, [], [
		AC_MSG_FAILURE('Failed to find libnetfilter-queue headers.')])

	AC_SEARCH_LIBS([nfq_open], [netfilter_queue], [], [
		AC_MSG_FAILURE('Failed to link with libnetfilter-queue.')])

	AC_SEARCH_LIBS([nfnl_open], [nfnetlink], [], [
		AC_MSG_FAILURE('Failed to link with libnfnetlink.')])
])

