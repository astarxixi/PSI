/*
 * psi_demo.cpp
 *
 *  Created on: May 20, 2015
 *      Author: mzohner
 */


#include "psi_demo.h"

int32_t main(int32_t argc, char** argv) {
	psi_demonstrator(argc, argv);
}

int32_t psi_demonstrator(int32_t argc, char** argv) {
	uint32_t nelements=0, elebytelen=16, symsecbits=128, intersect_size, i, j, ntasks=1, protocol=3,
			pnelements, *elebytelens, *res_bytelens;
	bool detailed_timings=false;
	uint8_t **elements, **intersection;
	string address="127.0.0.1";
	uint16_t port=7766;
	timeval t_start, t_end;
	vector<CSocket> sockfd(ntasks);
	string filename;
	uint64_t bytes_sent=0, bytes_received=0, mbfac;
	role_type role = (role_type) 0;
	double epsilon=1.2;

	mbfac=1024*1024;

	read_psi_demo_options(&argc, &argv, &role, &filename, &address, &nelements, &detailed_timings);

	if(role == SERVER) {
		listen(address.c_str(), port, sockfd.data(), ntasks);
	} else {
		for(i = 0; i < ntasks; i++)
			connect(address.c_str(), port, sockfd[i]);
	}

	gettimeofday(&t_start, NULL);

	//read in files and get elements and byte-length from there

	read_elements(&elements, &elebytelens, &nelements, filename);
	if(detailed_timings) {
		gettimeofday(&t_end, NULL);
	}

	pnelements = exchange_information(nelements, elebytelen, symsecbits, ntasks, protocol, sockfd[0]);
	//cout << "Performing private set-intersection between " << nelements << " and " << pnelements << " element sets" << endl;

	if(detailed_timings) {
		cout << "Time for reading elements:\t" << fixed << std::setprecision(2) << getMillies(t_start, t_end)/1000 << " s" << endl;
	}

	crypto crypto(symsecbits, (uint8_t*) const_seed);

	intersect_size = otpsi(role, nelements, pnelements, elebytelens, elements, &intersection, &res_bytelens,
			&crypto, sockfd.data(), ntasks, epsilon, detailed_timings);
	gettimeofday(&t_end, NULL);


#ifdef PRINT_INTERSECTION
	if(role == CLIENT) {
		//cout << "Computation finished. Found " << intersect_size << " intersecting elements:" << endl;
		if(!detailed_timings) {
			for(i = 0; i < intersect_size; i++) {
				//cout << "\t";
				for(j = 0; j < res_bytelens[i]; j++) {
					cout << intersection[i][j];
				}
				cout << endl;
			}
		}
	}
#endif

	for(i = 0; i < sockfd.size(); i++) {
		bytes_sent += sockfd[i].get_bytes_sent();
		bytes_received += sockfd[i].get_bytes_received();
	}

	//cout << "Required time:\t" << fixed << std::setprecision(1) << getMillies(t_start, t_end)/1000 << " s" << endl;
	//cout << "Data sent:\t" <<	((double)bytes_sent)/mbfac << " MB" << endl;
	//cout << "Data received:\t" << ((double)bytes_received)/mbfac << " MB" << endl;

	for(i = 0; i < nelements; i++)
		free(elements[i]);
	free(elements);
	free(elebytelens);
	return 1;
}


void read_elements(uint8_t*** elements, uint32_t** elebytelens, uint32_t* nelements, string filename) {
	uint32_t i, j;
	ifstream infile(filename.c_str());
	if(!infile.good()) {
		cerr << "Input file " << filename << " does not exist, program exiting!" << endl;
		exit(0);
	}
	string line;
	if(*nelements == 0) {
		while (std::getline(infile, line)) {
			++*nelements;
		}
	}
	*elements=(uint8_t**) malloc(sizeof(uint8_t*)*(*nelements));
	*elebytelens = (uint32_t*) malloc(sizeof(uint32_t) * (*nelements));

	infile.clear();
	infile.seekg(ios::beg);
	for(i = 0; i < *nelements; i++) {
		assert(std::getline(infile, line));
		(*elebytelens)[i] = line.length();
		(*elements)[i] = (uint8_t*) malloc((*elebytelens)[i]);
		memcpy((*elements)[i], (uint8_t*) line.c_str(), (*elebytelens)[i]);

#ifdef PRINT_INPUT_ELEMENTS
		cout << "Element " << i << ": ";
		for(j = 0; j < (*elebytelens)[i]; j++)
			cout << (*elements)[i][j];
		cout << endl;
#endif
	}
}




int32_t read_psi_demo_options(int32_t* argcp, char*** argvp, role_type* role, string* filename, string* address,
		uint32_t* nelements, bool* detailed_timings) {

	uint32_t int_role;
	parsing_ctx options[] = {{(void*) &int_role, T_NUM, 'r', "Role: 0/1", true, false},
			{(void*) filename, T_STR, 'f', "Input file", true, false},
			{(void*) address, T_STR, 'a', "IP-address", false, false},
			{(void*) nelements, T_NUM, 'n', "Num elements", false, false},
			{(void*) detailed_timings, T_FLAG, 't', "Flag: Detailed timings", false, false}
	};

	if(!parse_options(argcp, argvp, options, sizeof(options)/sizeof(parsing_ctx))) {
		print_usage(argvp[0][0], options, sizeof(options)/sizeof(parsing_ctx));
		cout << "Exiting" << endl;
		exit(0);
	}

	assert(int_role < 2);
	*role = (role_type) int_role;

	return 1;
}