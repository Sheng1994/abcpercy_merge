#pragma once
#ifndef PERCY_INTERFACE_H
#define PERCY_INTERFACE_H
#ifdef __cplusplus
extern "C" {
#endif
	ABC_NAMESPACE_IMPL_START

	// percy interface for external
	void percy_link(void);

	int percy_map(If_Cut_t * pCut);

	int percymapping_main(std::vector<std::vector<int>> nets, std::vector<int> pi, 
		std::vector<int> po, std::vector<int> gates, std::vector<std::string> tts_all);

	ABC_NAMESPACE_IMPL_END

#ifdef __cplusplus
}
#endif

#endif