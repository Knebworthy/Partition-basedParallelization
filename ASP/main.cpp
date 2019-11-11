//##########################################
//## MULTITHrest_numAD PROCESSING FOR CELL GROUPS
//##########################################

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

//=======================================================================================================================
//1. DEFINE TUNABLE PARAMETERS
//=======================================================================================================================


char geo_files[121]("D://xu//CHESS_PARALLEL//CHESS//xf_ws//geo//xf_ws");
//char geo_files[121]("D://xu//CHESS_PARALLEL//CHESS//lc//geo//lc");
//char geo_files[121]("Z://PARA//xf_ws//geo//xf_ws");
double  SDI(8);//Sub-basin division index, it will increase more sub-basins when it excels 1

double  Resolution(200);//m

//thread parameters
const int thread_num = 17;
int threads[thread_num] = {1,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32 };

double  MAX_DT = 0.4;//maximum DT
double  RDT = 0.05;// increment for next DT

double  PDT(0.2);//used in channel partition
double  SPCU(0.25);//smallest_portion_channel_unit
double  SR_GOAL_PER(1);


//other parameters (less important)
double  Stream_Boun_Area(0.5);
const int max_INF_array(10000);
struct inf *INF_array = new struct inf [max_INF_array];
struct inf *INF = new struct inf;

int layer;


using namespace std;

int main()
{
	int patch_num;
	int *ppatch_num=new int;
	char flowtable_in[121]{}, patch_in[121]{}, sthread_out[121]{}, cthread_out[121]{};
	strcpy_s(flowtable_in, geo_files);
	strcat_s(flowtable_in, "_flow_table_D8.dat");

	strcpy_s(sthread_out, geo_files);
	strcat_s(sthread_out, ".sthread");
	strcpy_s(cthread_out, geo_files);
	strcat_s(cthread_out, ".cthread");
	cout << SDI << endl;

	//read flow routing information
	struct patch_struct *patch = NULL;
	patch = read_flow_table(patch, flowtable_in, INF);
	patch_num = INF->patch_num;

	// allow for mutiple thread chioces
	for (int thread_inx = 0; thread_inx !=17; thread_inx++) {

		///*
		//=======================================================================================================================
		// SUB-BASIN PARTION AND THrest_numAD COMPOSITION
		//=======================================================================================================================
		int para_try_times = 0; 
		double DT = 0;//the first value of deviation_index
		do
		{
			init_hillslope_parallel(patch, INF);
			//generate a potential scheme by sub-bain partition
			hillslope_parallel(patch, DT, SDI, threads[thread_inx], INF,0);

			cout << "\t\t\t " << DT << "\t"  << fixed << setprecision(2) << INF->HSR << "\t" << fixed << setprecision(2) << INF->subbasin_num  << endl;

			//init
			DT += RDT;
			INF_array[para_try_times] = INF[0];
			para_try_times++;

		} while (DT < MAX_DT);
		
		//select optimal result in terms of hsr
		double max_sr = 0;
		int best_INF_inx = 0;
		for (int inx = 0; inx != para_try_times; inx++)
			if (INF_array[inx].HSR > max_sr) {
				max_sr = INF_array[inx].HSR;
				best_INF_inx = inx;
			}
		DT = best_INF_inx * RDT;
		
		//fianl scheme with final DT
		init_hillslope_parallel(patch, INF);		
		hillslope_parallel(patch, DT, SDI, threads[thread_inx], INF,0);

		cout << "\t\t\t " << DT << "\t" << fixed << setprecision(2) << INF->HSR << "\t" << fixed << setprecision(2) << INF->subbasin_num << endl;


		//WRITE STHrest_numAD FILE 
		print_threads_map(patch, geo_files, sthread_out, patch_num, threads[thread_inx], 1);
		//*/

		//=======================================================================================================================
		// CHANNEL PARTION AND THrest_numAD COMPOSITION
		//=======================================================================================================================
		double SR_GOAL = threads[thread_inx] * SR_GOAL_PER;		
		layer = 0;
		init_channel_parallel(patch, INF);
		do
		{
			//init
			DT = 0;
			int layer_try_times = 0;

			//iteration in each layer
			int out_flag = 0;
			do{

				//computing all informations
				channel_parallel(patch, DT, threads[thread_inx], INF,0);
				
				//cout informations
				//cout << "\t " << layer <<"\t"<<layer_try_times<< "\t" << fixed << setprecision(2) << DT << "\t" << fixed << setprecision(2) << INF->SR
					//<< "\t" <<INF->channel_unit_num << "\t" << INF->partitioned_num << "\t" << INF->rest_num << "\t" << INF->CN << endl;
				
				//init as in original state
				update_channel_parallel_within_layer(patch, patch_num, layer, 0);

				//store out information
				INF_array[layer_try_times]=INF[0];

				//move to next layer try
				DT = (1+DT)*(1+RDT)-1;
				
				if (layer_try_times > 1) {
					if (INF_array[layer_try_times].SR - INF_array[layer_try_times - 1].SR < 0)
						out_flag = 1;
					else if (INF->rest_num == 0)
						out_flag = 1;
				}

				layer_try_times++;
			} while (out_flag==0);//


			//select optimal result by goals
			double max_sr = 0;
			double max_pn = 0;
			int best_INF_inx=0;
			//compute max sr
			for (int inx = 0; inx != layer_try_times; inx++) {
				
				if (INF_array[inx].SR > max_sr){
					max_sr = INF_array[inx].SR;
					best_INF_inx = inx;
				}
			}
			//[partitioned_num first]
			if (max_sr >= SR_GOAL) {
				for (int inx = 0; inx != layer_try_times; inx++) {
					//search_max_sr with max_pn
					if (INF_array[inx].SR >= SR_GOAL && INF_array[inx].partitioned_num > max_pn) {//excels to SR_GOAL with max partitioned_num
						max_pn = INF_array[inx].partitioned_num;
						best_INF_inx = inx;
					}
				}
			}
			//[SR first]
			else {
			
				for (int inx = 0; inx != layer_try_times; inx++) {
					if (fabs(INF_array[inx].SR-max_sr)<0.0001 && INF_array[inx].partitioned_num > max_pn) {//equals to MAX_SR with max partitioned_num
						max_pn = INF_array[inx].partitioned_num;
						best_INF_inx = inx;
					}
				}
			
			}

			//renew layer result
			DT = 0;
			for (int inx = 0; inx != best_INF_inx;inx++)
				DT = (1 + DT)*(1 + RDT)-1;

			channel_parallel(patch, DT, threads[thread_inx], INF,0);

			//cout informations
			cout << "\t " << layer << "\t" << best_INF_inx <<"\t"<< fixed << setprecision(2) << DT << "\t" << fixed << setprecision(2) << INF->SR
				<< "\t" << INF->channel_unit_num << "\t" << INF->partitioned_num << "\t" << INF->rest_num << "\t" << INF->CN << endl;

			//as static
			update_channel_parallel_within_layer(patch, patch_num, layer, 1);
			INF->partitioned_num_CH[layer] = INF->partitioned_num;
			INF->SR_CH[layer] = INF->SR;
			layer++;

			if (layer == 1) {
				int a = 0;
				//break;
			}

		} while (INF->rest_num!=0);
		INF->LAYER = layer;

		//print channel thread map
		//print_threads_map(patch, geo_files, cthread_out, patch_num, threads[thread_inx], 2);

		//compute final CSR
		for (int inx = 0; inx != INF->LAYER; inx++) {
		
			INF->CSR += INF->partitioned_num_CH[inx] / INF->SR_CH[inx];
		}
		INF->CSR = INF->channel_num/ INF->CSR;

		//print results
		
		INF->SR = patch_num / ((patch_num - INF->channel_num) / INF->HSR + INF->channel_num / INF->CSR);
		
		cout << "\tTHrest_numAD " << threads[thread_inx] << "\t End with " <<  fixed << setprecision(2) << INF->SR<<  "\t"<<
			fixed << setprecision(2) << INF->HSR << "\t" << patch_num- INF->CN << "\t" <<
			fixed << setprecision(2) << INF->CSR << "\t" << INF->CN << "\t\n" << endl;

	}

	

	cout << "\nINFPUT IS AVAILABLE NOW...." << endl;
	getchar();

	return 0;
}