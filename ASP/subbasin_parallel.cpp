//---------------------------------------------------------------------------------------------------------------------------
//MAJOR FUNCTION OF ASP
//---------------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>
#include "PARA.h"

using namespace std;

void subbasin_parallel(struct patch_struct *patch, double Target_Deviation,int Division_Times,  int threads,  int patch_num,double Resolution, double Stream_Boun_Area, struct out_inf* Out_Inf,int out_flag) {

	//=======================================================================================================================
	//1. LOCAL VARS(JUST SKIP IF YOU DONT WANT TO READ ABOUT IT)
	//=======================================================================================================================

	const int infinite = 99999999;
	// while partitioning, num of subbasins will be more than the basin_num we want
	// thus we need more memory for storage and by using optimize method we finally got sufficient basin_num
	const int sub_basin_times(20);

	int itmp{};
	int precision{};
	int first_round{};
	int round_flag{};
	int round_total{};
	int optimize_flag{};
	int new_main_basin_flag{};
	int best_subbasin_pch_num{};
	int best_subbasin_pch_ID{};
	int best_deviation{};
	int min_subbasin_pch_num{};
	int min_subbasin_pch_ID{};
	int max_flag{};
	int max_basin_ID{};
	int max_subbasin_ID_OUT{};
	int max_basin_pch_num{};
	int optimize_total{};
	int break_flag{};

	double stream_boun = Stream_Boun_Area * 1000 * 1000 / (Resolution*Resolution);

	// the limit of acc_area of a new basin
	// the most important index in ASP process
	// Target ACC AREA
	double AT = patch_num / (Division_Times *threads) * (1 + Target_Deviation);

	//SUB_BASINS CAUGHT IN STEP 1 
	int **basin_pches = new int *[sub_basin_times * threads];
	for (int i = 0; i < sub_basin_times * threads; ++i)
	{
		basin_pches[i] = new int[patch_num]{};
	}


	//SUB-BASIN_ID IN FINAL THREADS
	int *basin_inx_ID = new int[sub_basin_times * threads]{};
	int *basin_pch_num = new int[sub_basin_times * threads]{};

	//ID of subbasins in a basin 
	int **final_subbasin_collection = new int *[threads] {};
	for (int i = 0; i < threads; ++i)
	{
		final_subbasin_collection[i] = new int[threads*sub_basin_times]{};
	}	
	//num of patches in a subbasin 
	int **final_subbasin_pch_num = new int *[threads] {};
	for (int i = 0; i < threads; ++i)
	{
		final_subbasin_pch_num[i] = new int[threads*sub_basin_times]{};
	}

	int *final_basin_pch_num = new int [threads] {};
	//num of patches in a basin
	int *final_pch_num = new int [threads] {};
	//num of subbasins in a basin
	int *final_subbasin_num = new int [threads] {};


	//=======================================================================================================================
	//2. STEP 1:SEARCHING FOR SINGLE BASINS
	//=======================================================================================================================

	// the ID of basin num 
	int basin_inx = 0;//start from 0 and controls the num of subbasins (before we optimize it)
	int rest_num{};//rest of num undistributed with their basin_inx

	//=======================================================================================================================
	//2. STEP 1:SUB-BASIN PARTITION
	//=======================================================================================================================
	//rest patches waited for distribution
	rest_num = patch_num;

	//START PARTITIONING
	do {


		new_main_basin_flag = 0;

		//INIT
		best_subbasin_pch_num = 0;
		best_subbasin_pch_ID = 0;
		best_deviation = infinite;

		//---------------------------------------------------------------------------------------------------------------------------
		// START SEACHING FOR IDEAL SUBBAISN ASSEMBLE FROM THE HIGHEST PATCH 
		//---------------------------------------------------------------------------------------------------------------------------
		for (int pch = 0; pch != patch_num; pch++) {

			//stream patches that are remained 
			if (patch[pch].acc_area > stream_boun && patch[pch].basin_inx == 0)
			{
				//find best ID
				if (fabs(patch[pch].re_acc_area - AT)< best_deviation && patch[pch].re_acc_area - AT<0) {

						best_subbasin_pch_num = basin_pch_num[basin_inx];
						best_subbasin_pch_ID = pch;
						best_deviation = fabs(patch[pch].re_acc_area - AT);
				}
			}
		}
		//END OF SERCHING  

		//---------------------------------------------------------------------------------------------------------------------------
		//STARTING DISTRIBUTION AND RENEW re_acc_area
		//---------------------------------------------------------------------------------------------------------------------------

		patch[best_subbasin_pch_ID].basin_inx = basin_inx + 1;//start from 1

		//add first ID in the list
		basin_pches[basin_inx][(basin_pch_num[basin_inx])] = patch[best_subbasin_pch_ID].patchID;

		//add boundary
		basin_pch_num[basin_inx]++;

		//searching for upslope undefined patches
		for (int up_pch_inx = best_subbasin_pch_ID - 1; up_pch_inx >= 0; up_pch_inx--) {

			// only when it's unlabelled
			if (patch[up_pch_inx].basin_inx == 0) {
				//check it's neigh
				for (int neigh = 0; neigh != patch[up_pch_inx].neigh_num; neigh++) {

					//check the list now in this basins
					for (int basin_pch_inx = 0; basin_pch_inx != basin_pch_num[basin_inx]; basin_pch_inx++) {

						//when it's neigh is in the basin list
						if (patch[up_pch_inx].neigh_ID[neigh] == basin_pches[basin_inx][basin_pch_inx]) {

							//add label
							patch[up_pch_inx].basin_inx = basin_inx + 1;//start from 1

							//add it's ID in the basin list
							basin_pches[basin_inx][(basin_pch_num[basin_inx])] = patch[up_pch_inx].patchID;
							basin_pch_num[basin_inx]++;
							break;
						}
					}

				}
			}

		}//end of seaching it's upslope area

		//renew re_acc_area
		for (int pch = 0; pch != patch_num; pch++) {
			//found the outlet
			if (patch[pch].patchID == basin_pches[basin_inx][0]) {
				// in an desending order to to final basin outlet
				int up_pch = 0;
				int down_pch = pch;
				while (down_pch != patch_num - 1) {

					up_pch = down_pch;

					//search for down pch
					for (down_pch = up_pch - 1; down_pch != patch_num; down_pch++) {
						if (patch[down_pch].patchID == patch[up_pch].neigh_ID[0])
							break;
					}

					patch[down_pch].re_acc_area -= basin_pch_num[basin_inx];

				} //till outlet
				break;//for route out another basin
			}
		}
		//after divided
		rest_num -= basin_pch_num[basin_inx];
		basin_inx++;
		//cout << basin_inx << "\t" << basin_pch_num[basin_inx - 1] << "\t" << rest_num << endl;
		//---------------------------------------------------------------------------------------------------------------------------
		// END OF COMPASATION PROCESS FOR FINDING SUBBASINS
		//---------------------------------------------------------------------------------------------------------------------------

		if (out_flag != 0)
			rest_num = 0;


	} while (rest_num != 0);


	Out_Inf->SB = basin_inx;


	//=======================================================================================================================
	//4. STEP 3:m sub-basins into final_threads 
	//=======================================================================================================================

	//add basin_ID
	for (int inx = 0; inx < basin_inx; inx++)
		basin_inx_ID[inx] = inx;

	//sort basins
	for (int inx = 0; inx < basin_inx; inx++)
	{
		for (int iny = inx + 1; iny < basin_inx; iny++) {

			//exchange lower values
			if (basin_pch_num[inx] < basin_pch_num[iny]) {


				//exchange their ID
				for (int pch = 0; pch != basin_pch_num[iny]; pch++)
				{
					itmp = basin_pches[iny][pch];
					basin_pches[iny][pch] = basin_pches[inx][pch];
					basin_pches[inx][pch] = itmp;

				}

				//exchange their num
				itmp = basin_pch_num[iny];
				basin_pch_num[iny] = basin_pch_num[inx];
				basin_pch_num[inx] = itmp;

				//exchange ID
				itmp = basin_inx_ID[iny];
				basin_inx_ID[iny] = basin_inx_ID[inx];
				basin_inx_ID[inx] = itmp;

			}
		}
	}

	// put basins in the final basin assemble
	int final_basin_ID{};
	for (int inx = 0; inx < basin_inx; inx++) {

		final_basin_ID = inx % threads;
		final_subbasin_collection[final_basin_ID][(final_subbasin_num[final_basin_ID])] = basin_inx_ID[inx];
		final_subbasin_pch_num[final_basin_ID][(final_subbasin_num[final_basin_ID])] = basin_pch_num[inx];
		final_subbasin_num[final_basin_ID]++;
		final_pch_num[final_basin_ID] += basin_pch_num[inx];
	}

	//main optimizing process to search for an ideal subbasin assemble to final basin
	do {

		//reinit as 0
		max_basin_ID = min_subbasin_pch_num = max_subbasin_ID_OUT = max_basin_pch_num = 0;

		//find maxbasin_out
		for (int inx = 0; inx != threads; inx++) {

			if (final_pch_num[inx] > max_basin_pch_num) {
				max_basin_pch_num = final_pch_num[inx]; max_basin_ID = inx;
			}
		}
		//find it's smallest subbasin
		for (int iny = 0; iny != final_subbasin_num[max_basin_ID]; iny++) {

			if (iny == 0) {

				min_subbasin_pch_num = final_subbasin_pch_num[max_basin_ID][iny];
				max_subbasin_ID_OUT = iny;

			}

			if (final_subbasin_pch_num[max_basin_ID][iny] < min_subbasin_pch_num) {

				min_subbasin_pch_num = final_subbasin_pch_num[max_basin_ID][iny];
				max_subbasin_ID_OUT = iny;
			}

		}

		//check if we can optimize it 
		//by adding it to one of other basins
		optimize_flag = 0;

		for (int inx = 0; inx != threads; inx++) {

			//only in other basins
			if (inx != max_basin_ID) {

				if (final_pch_num[inx] + min_subbasin_pch_num < final_pch_num[max_basin_ID]) {

					//remove smallest subbasins into this basin

					//an -1 as it's ID is from 0
					final_subbasin_collection[inx][(final_subbasin_num[inx])] = final_subbasin_collection[max_basin_ID][max_subbasin_ID_OUT];
					final_subbasin_pch_num[inx][(final_subbasin_num[inx])] = final_subbasin_pch_num[max_basin_ID][max_subbasin_ID_OUT];

					final_subbasin_collection[max_basin_ID][max_subbasin_ID_OUT] = 0;
					final_subbasin_pch_num[max_basin_ID][max_subbasin_ID_OUT] = 0;

					//need to reorder the list of subbasins in this OUT basin
					for (int ainx = 0; ainx != final_subbasin_num[max_basin_ID]; ainx++)
						for (int binx = ainx + 1; binx != final_subbasin_num[max_basin_ID]; binx++) {

							if (final_subbasin_pch_num[max_basin_ID][ainx] < final_subbasin_pch_num[max_basin_ID][binx]) {

								//exchange value
								itmp = final_subbasin_collection[max_basin_ID][ainx];
								final_subbasin_collection[max_basin_ID][ainx] = final_subbasin_collection[max_basin_ID][binx];
								final_subbasin_collection[max_basin_ID][binx] = itmp;

								itmp = final_subbasin_pch_num[max_basin_ID][ainx];
								final_subbasin_pch_num[max_basin_ID][ainx] = final_subbasin_pch_num[max_basin_ID][binx];
								final_subbasin_pch_num[max_basin_ID][binx] = itmp;

							}
						}

					//then we can delete the value
					final_subbasin_num[inx]++;
					final_subbasin_num[max_basin_ID]--;
					final_pch_num[inx] += min_subbasin_pch_num;
					final_pch_num[max_basin_ID] -= min_subbasin_pch_num;

					optimize_flag = 1;
					optimize_total++;

					//cout << "round\t" << optimize_total << endl;

					//need to break 
					break;

				}
			}
		}


	} while (optimize_flag == 1);// till we got fine size of final basins or it can not be optimized


	//correlating final basins with final thread
	for (int pch = 0; pch != patch_num; pch++) {

		//search
		break_flag = 0;
		for (int basin_inx = 0; basin_inx != threads; basin_inx++)
		{

			for (int sub_basin_inx = 0; sub_basin_inx != final_subbasin_num[basin_inx]; sub_basin_inx++) {

				if (patch[pch].basin_inx == (final_subbasin_collection[basin_inx][sub_basin_inx] + 1)) {

					//start from 1
					//patch[pch].basin_inx = basin_inx + 1; do not update any more
					patch[pch].sthread = basin_inx + 1;
					
					final_basin_pch_num[basin_inx]++;

					//break in this for and next for
					break_flag = 1;
					break;

				}

			}
			//which means we has searched a good point
			if (break_flag == 1) break;

		}
		if (break_flag != 1) cout << "UNMATCHED PATCH WITH THEIR BASIN INX" << endl;
	}

	//CHECK DF
	double Final_Deviation = 0;
	for (basin_inx = 0; basin_inx != threads; basin_inx++) {
		if (final_basin_pch_num[basin_inx] > Final_Deviation)
			Final_Deviation = final_basin_pch_num[basin_inx];
	}
	Final_Deviation = Final_Deviation / (patch_num / threads) - 1;


	//FREE MEMORY
	for (int i = 0; i < sub_basin_times * threads; ++i)
			delete [] basin_pches[i];
	delete[] basin_pches;

	delete[] basin_inx_ID;
	delete[] basin_pch_num;

	for (int i = 0; i < threads; ++i)
		delete[] final_subbasin_collection[i];
	delete[] final_subbasin_collection;
	
	for (int i = 0; i < threads; ++i)
		delete[]  final_subbasin_pch_num[i];
	delete[] final_subbasin_pch_num;

	delete[] final_basin_pch_num;
	delete[] final_pch_num;
	delete[] final_subbasin_num;

	Out_Inf->DF = Final_Deviation;
	Out_Inf->SR = 1/(1+Final_Deviation)*threads;

	return;
}//END OF ASP