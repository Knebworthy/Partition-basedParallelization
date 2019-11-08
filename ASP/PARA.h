
//FlowTable Structure
struct	patch_struct {

	int pch;//the ID in descending order, start from 0

	int	patchID;
	int  landID;
	int	 inx;
	double lat;
	double lon;
	double z;
	double acc_area;

	//remained acc_area
	double re_acc_area;

	double runoff_coefficient;
	
	//this has been read 
	int    neigh_num{};
	int    neigh_ID[8]{};
	double neigh_gamma[8]{};//neighbor
	int	   neigh_pch[8]{};


	//need to be init here
	int    above_num{};
	int    above_ID[8]{};
	double above_gamma[8]{};//neighbor
	int	   above_pch[8]{};

	//new added data 
	int basin_inx{}; // ID of basins units
	int sthread{}; //sub-basin thread
	
	// channel layer and thread
	// e.g. 1000100 means the 1000 layer and the 100 thread
	int cthread{}; 
	int channel_inx{};//ID of channel units
	int layer{};//1000 the layer
	int layer_thread{};//100 the corresponding thread

	//state of channel_inx
	int channel_state{};//1 transient, 2 static 0 not valued
	int lock_state{};//0 unlocked, 1 locked 

	// patch 1 and 2 flow to 3, 1 and 2 are two outlets and 3 is a convergent point
	int outlet_flag{};
	int converge_flag{};

	//steam length
	int longest_stream_length;

	//channel acc
	int channel_acc;
	int hill_acc;
	int re_channel_acc;
	int re_channel_acc_backup;
};

//OUT_INF
struct out_inf {

	double DF{};
	int	   SB{};//sub-basins
	
	double SR_HI = {};
	double SI_NUM = {};
	double SR_CH[256] = {};
	double PN_CH[256] = {};
	double LAYER = {};
	double CSR = {};
	double HSR = {};


	//for each try
	double SR{};//speed up ratio
	int    CUN{};//channel unit num
	int    CN{};//channel num
	int	   PN{};//partitioned channel num in this layer
	int    RE{};//rest channel num
	int    RE_CH[256]{};


};


//Functions

struct patch_struct* read_flow_table(struct patch_struct *patch, char *flowtable_in,int*ppatch_num);
void print_threads_map(struct patch_struct *patch, char *patch_in, char *threads_out, int patch_num, int thread, int print_flag);
void init_subbasin_parallel(struct patch_struct *patch, int patch_num);
void init_channel_parallel(struct patch_struct *patch, int patch_num, int layer, int init_flag);
void subbasin_parallel(struct patch_struct *patch, double Target_Deviation,int Division_Times, int threads, 
						int patch_num, double Resolution, double Stream_Boun_Area, struct out_inf* Out_Inf,int out_flag);
void channel_parallel(struct patch_struct *patch, double Target_Deviation, int Division_Times, int threads,
	int patch_num, double Resolution, double Stream_Boun_Area, struct out_inf* Out_Inf,int out_flag);

