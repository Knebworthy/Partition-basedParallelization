
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
	double longest_stream_length{};

	//accumulated runtime
	int channel_acc;//original value 
	int hill_acc;//original value 
	
	int re_hill_acc;//remained value 

	int re_channel_acc;//remained value 
	int re_channel_acc_backup;//the backup is used to store the re_channel_acc while trying different parameters: tchm
};

//OUT_INF
struct inf {

	int patch_num{};
	int hillslope_num{};
	int channel_num{};

	double DF{};
	int	   subbasin_num{};//sub-basins
	
	double SR_CH[256] = {};
	double partitioned_num_CH[256] = {};
	double LAYER = {};
	double CSR = {};
	double HSR = {};




	//for each try
	double SR{};//speed up ratio
	int    channel_unit_num{};//channel unit num
	int    CN{};//channel num
	int	   partitioned_num{};//partitioned channel num in this layer
	int    rest_num{};//rest channel num
	double    TAi[256]{};

	double tmsr_ch;
	double tmsr_hi;

};


//Functions

struct patch_struct* read_flow_table(struct patch_struct *patch, char *flowtable_in, struct inf* Inf);
void print_threads_map(struct patch_struct *patch, char *patch_in, char *threads_out, int patch_num, int thread, int print_flag);
void init_hillslope_parallel(struct patch_struct *patch, struct inf* Inf);
void init_channel_parallel(struct patch_struct *patch, struct inf *INF);
void update_channel_parallel_within_layer(struct patch_struct *patch, int patch_num, int layer, int init_flag);
void hillslope_parallel(struct patch_struct *patch, double Deviation_Index, int Decomposition_Index, int threads, struct inf *INF, int out_flag);

void channel_parallel(struct patch_struct *patch, double deviation_index, int threads, struct inf* INF, int out_flag);

