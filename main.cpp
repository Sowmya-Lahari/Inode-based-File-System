#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[01;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[0m"
#define DISK_SIZE 500000000
//#define DISK_SIZE 5000
#define BLOCK_SIZE 500
#define INODE_SIZE 40
#define NUM_BLOCKS DISK_SIZE/BLOCK_SIZE
#define SUPER_BLOCKS 32
#define INODE_BLOCKS 4
#define INODES_PER_BLOCK BLOCK_SIZE/INODE_SIZE
#define INODE_COUNT INODE_BLOCKS*INODES_PER_BLOCK
#define IMAP_OFFSET 32
#define DMAP_OFFSET IMAP_OFFSET + INODE_COUNT
#define DATA_BLOCKS NUM_BLOCKS - INODE_BLOCKS
#define DATA_OFFSET (INODE_BLOCKS + SUPER_BLOCKS) * BLOCK_SIZE
#define FNAME_SZ 10
#define F_SZ 10
#define PTR_SZ 10
#define F_BLK_SZ 10
using namespace std;

struct inode
{
    long long add;
    long long size;
    long long blocks;
}my_inode;

string replace(string s)
{
    for(long long i=0; i<s.size(); i++)
        if(s[i] == '$')
            s[i] = '\n';
    return s;
}

// void print()
// {
//     printf("NUM_BLOCKS: %lli\n",DISK_SIZE/BLOCK_SIZE);
//     printf("INODE_BLOCKS: 1\n");
//     printf("INODES_PER_BLOCK: %lli\n", BLOCK_SIZE/INODE_SIZE);
//     printf("INODE_COUNT: %lli\n", INODE_BLOCKS*INODES_PER_BLOCK);
//     printf("IMAP_OFFSET: 32\n");
//     printf("DMAP_OFFSET: %lli\n", IMAP_OFFSET + INODE_BLOCKS);
//     printf("DATA_BLOCKS: %lli\n", NUM_BLOCKS - INODE_BLOCKS);
//     printf("DATA_OFFSET: %lli\n", (INODE_BLOCKS + SUPER_BLOCKS) * BLOCK_SIZE);
// }
string get_inode(string fname, long long add, long long size, long long num)
{
    fname.resize(FNAME_SZ, '$');
    string st_add = to_string(add);
    st_add.resize(PTR_SZ, '$');
    string st_sz = to_string(size);
    st_sz.resize(F_SZ, '$');
    string st_num = to_string(num);
    st_num.resize(F_BLK_SZ, '$');
    return fname + st_add + st_sz + st_num;
}

struct inode get_details(string inode_data)
{
    inode_data = inode_data.substr(FNAME_SZ);
    // //cout<<inode_data<<endl;
    string temp_add = inode_data.substr(0, PTR_SZ);
    inode_data = inode_data.substr(PTR_SZ);
    // //cout<<inode_data<<endl;
    temp_add = temp_add.substr(0,temp_add.find_first_of('$'));
    string temp_fsz = inode_data.substr(0, F_SZ);
    temp_fsz = temp_fsz.substr(0,temp_fsz.find_first_of('$'));
    inode_data = inode_data.substr(F_SZ);
    // //cout<<inode_data<<endl;
    string temp_num_blk = inode_data;   
    temp_num_blk = temp_num_blk.substr(0,temp_num_blk.find_first_of('$'));
    // //cout<<"Extracted: "<<temp_add<<" "<<temp_fsz<<" "<<temp_num_blk<<endl;
    long long num_blk = stoll(temp_num_blk);
    long long cur_add = stoll(temp_add);
    struct inode new_node;
    new_node.add = cur_add;
    new_node.size = stoll(temp_fsz);
    new_node.blocks = num_blk;
    return new_node;
}
long long fd = 0;
int main()
{
    vector <string> disk_list;
    map <string, pair<long long,long long>> file_dets;
    map <string, pair<long long,long long>> open_files;
    map <long long, pair<string,long long>> file_descr;
    map <string, vector<long long>> file_blocks;
    string inode_bitmap;
    string block_bitmap;
    long long size_data = 0;
    map <long long, string> mode_map;
    mode_map[0] = "Read";
    mode_map[1] = "Write";
    mode_map[2] = "Append";
    long long ch = 1;
    while(ch)
    {
        printf("%sDisk Menu\n", BLUE);
        printf("%s1. Create Disk\n", YELLOW);
        printf("%s2. Mount Disk\n", YELLOW);
        printf("%s3. Exit\n", YELLOW);
        printf("%sEnter Your Choice: ", WHITE);
        cin>>ch;
        if (ch == 1)
        {
            string name;
            printf("%sEnter the name of the disk: ", WHITE);
            cin>>name;
            name = name + ".txt";
            ofstream new_disk;
            //Create new disk
            new_disk.open(name);
            new_disk.seekp(DISK_SIZE-1);
            new_disk.write("", 1);
            new_disk.clear();
            
            //Initialize the super block
            new_disk.seekp(0);
            string numb = to_string(NUM_BLOCKS) + "," + to_string(INODE_BLOCKS);
            new_disk.write(numb.c_str(), numb.size());
            size_data = numb.size();
            new_disk.seekp(IMAP_OFFSET);
            // //cout<<new_disk.tellp()<<endl;
            inode_bitmap = "";
            for(long long i=0;i<INODE_COUNT;i++)
                inode_bitmap += "0";
            new_disk.write(inode_bitmap.c_str(), inode_bitmap.size());
            // //cout<<INODE_COUNT<<endl;
            block_bitmap="";
            new_disk.seekp(DMAP_OFFSET);
            for (long long i = 0; i < DATA_BLOCKS; i++)
                block_bitmap += "0";
            new_disk.write(block_bitmap.c_str(), block_bitmap.size());
            // //cout<<DATA_BLOCKS<<endl;

            printf("%s%s is created successfully!!!\n", GREEN, name.substr(0,name.find_first_of('.')).c_str());
            // print();
            disk_list.push_back(name);
        }
        else if(ch == 2) //mount
        {
            fd = 0;
            string dname;
            printf("%sEnter the name of the disk to be mounted: ", WHITE);
            cin>>dname;
            dname += ".txt";
            if (find(disk_list.begin(), disk_list.end(), dname) == disk_list.end())
                printf("%sInvalid Disk name!!!\n", RED);
            else
            {
                fstream disk;
                disk.open(dname);
                printf("%s%s mounted successfully\n", GREEN, dname.substr(0,dname.find_first_of('.')).c_str());

                //Read super block data
                char temp[INODE_COUNT + DATA_BLOCKS];
                disk.seekg(IMAP_OFFSET);
                // //cout<<disk.tellg()<<endl;
                // //cout<<INODE_COUNT + DATA_BLOCKS<<" "<<IMAP_OFFSET<<endl;
                disk.read(temp, INODE_COUNT + NUM_BLOCKS);
                temp[INODE_COUNT + DATA_BLOCKS] = '\0';
                string sizes = temp;
                // //cout<<sizes<<endl;
                inode_bitmap = sizes.substr(0,INODE_COUNT);
                block_bitmap = sizes.substr(INODE_COUNT);
                //cout<<inode_bitmap.size()<<"   "<<block_bitmap.size()<<" "<<sizes.size()<<endl;
                long long base = BLOCK_SIZE;
                long long incr = INODE_SIZE;

                //Read inode details
                for(long long p=0; p<INODE_COUNT; p++)
                {
                    if(inode_bitmap[p] == '1')
                    {
                        disk.seekp(base+p*incr);
                        char temp_inode[INODE_SIZE];
                        disk.read(temp_inode, INODE_SIZE);
                        string cur_inode = temp_inode;
                        string cur_fname = cur_inode.substr(0,cur_inode.substr(0,8).find_first_of('$'));
                        file_dets[cur_fname] = make_pair(base+p*incr, fd++);
                        file_descr[fd-1] = make_pair(cur_fname, -2);
                    }
                }
                
                long long ch2 = 1;
                while(ch2)
                {
                    // print();
                    long long rem_blocks = DATA_BLOCKS;
                    printf("%sMounting Menu\n", BLUE);
                    printf("%s1. Create file\n", YELLOW);
                    printf("%s2. Open file\n", YELLOW);
                    printf("%s3. Read file\n", YELLOW);
                    printf("%s4. Write file\n", YELLOW);
                    printf("%s5. Append file\n", YELLOW);
                    printf("%s6. Close file\n", YELLOW);
                    printf("%s7. Delete file\n", YELLOW);
                    printf("%s8. Display List of files\n", YELLOW);
                    printf("%s9. Display List of opened files\n", YELLOW);
                    printf("%s10. Unmount\n", YELLOW);
                    printf("%sEnter Your Choice: ", WHITE);
                    cin>>ch2;
                    long num_blocks = DISK_SIZE/BLOCK_SIZE;
                    // long rem_blocks = num_blocks;
                    if (ch2 == 1) //Create
                    {
                        if(rem_blocks == 0)
                            printf("%sDisk is Full!!\n", RED);
                        else
                        {
                            string fname;
                            printf("Enter the name of the file: ");
                            cin>>fname;
                            long long empty_block = block_bitmap.find('0');
                            if(empty_block == -1)
                            {
                                printf("%sDisk is full!!", RED);
                                continue;
                            }

                            //Write inode data to file
                            long long empty_inode = inode_bitmap.find('0');
                            file_dets[fname] = make_pair(BLOCK_SIZE + empty_inode * INODE_SIZE, fd);
                            file_descr[fd] = make_pair(fname, -1);
                            // //cout<<"inode_addr of "<<fname<<" "<<file_dets[fname].first<<endl;
                            inode_bitmap[empty_inode] = '1';
                            string cur_inode_data;
                            fname.resize(FNAME_SZ,'$');
                            cur_inode_data = fname;
                            cur_inode_data += to_string((INODE_BLOCKS+SUPER_BLOCKS+empty_block)*BLOCK_SIZE);
                            file_blocks[fname].push_back(empty_block);
                            // //cout<<cur_inode_data<<endl;
                            // //cout<<"Address assigned :"<<((INODE_BLOCKS+1+empty_block)*BLOCK_SIZE)<<endl;
                            cur_inode_data.resize(FNAME_SZ + PTR_SZ,'$');
                            // //cout<<cur_inode_data<<endl;
                            cur_inode_data += to_string(BLOCK_SIZE);
                            // //cout<<cur_inode_data<<endl;
                            cur_inode_data.resize(FNAME_SZ + PTR_SZ + F_SZ,'$');
                            // //cout<<cur_inode_data<<endl;
                            cur_inode_data += '1';
                            // //cout<<cur_inode_data<<endl;
                            cur_inode_data.resize(INODE_SIZE, '$');
                            // //cout<<cur_inode_data<<endl;
                            disk.seekp(BLOCK_SIZE + empty_inode * INODE_SIZE);
                            // //cout<<cur_inode_data<<endl;
                            disk.write(cur_inode_data.c_str(), cur_inode_data.size());
                            block_bitmap [empty_block] = '1';
                            // open_files[fname] = make_pair(fd, -1);
                            printf("%s%s created successfully\n", GREEN, fname.substr(0,fname.find_first_of('$')).c_str());
                            rem_blocks++;
                            fd++;
                        }
                    }
                    else if (ch2 == 2) //Open
                    {
                        printf("%sEnter file name: ",WHITE);
                        string fname;
                        cin>>fname;
                        if(file_dets.find(fname) == file_dets.end())
                        {
                            printf("%sFile does not exists!\n", RED);
                            continue;
                        }
                        long long mode;
                        printf("%s0: Read Mode\n1: Write Mode\n2: Append Mode\nEnter your choice: ", WHITE);
                        cin>>mode;
                        long long cur_fd = file_dets[fname].second;
                        file_descr[cur_fd].second = mode;
                        open_files[fname] = make_pair(cur_fd, mode);
                        printf("%sFile opened successfully in %s mode with file descriptor %lli\n", GREEN, mode_map[mode].c_str(), file_dets[fname].second);
                    }
                    else if(ch2 == 3) //Read
                    {
                        long long cur_fd;
                        printf("%sEnter file descriptor: ",WHITE);
                        cin>>cur_fd;
                        if(file_descr.find(cur_fd)==file_descr.end())
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second != 0 && file_descr[cur_fd].second > 0)
                        {
                            printf("%sFile already opened in other mode!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second < 0)
                        {
                            printf("%sFile not opened!\n", RED);
                            continue;
                        }
                        string fname = file_descr[cur_fd].first;
                        // cout<<"Reading file "<<fname<<endl;
                        long long inode_add = file_dets[fname].first;
                        disk.seekg(inode_add);
                        // cout<<"Inode address: "<<inode_add<<endl;
                        char temp[INODE_SIZE];
                        disk.read(temp,INODE_SIZE);
                        // cout<<"Inode data: "<<temp<<endl;
                        string inode_data = temp;
                        inode_data = inode_data.substr(FNAME_SZ);
                        // cout<<inode_data<<endl;
                        string temp_add = inode_data.substr(0, PTR_SZ);
                        inode_data = inode_data.substr(PTR_SZ);
                        // cout<<inode_data<<endl;
                        temp_add = temp_add.substr(0,temp_add.find_first_of('$'));
                        string temp_fsz = inode_data.substr(0, F_SZ);
                        temp_fsz = temp_fsz.substr(0,temp_fsz.find_first_of('$'));
                        inode_data = inode_data.substr(F_SZ);
                        // cout<<inode_data<<endl;
                        string temp_num_blk = inode_data;   
                        temp_num_blk = temp_num_blk.substr(0,temp_num_blk.find_first_of('$'));
                        //cout<<"Extracted: "<<temp_add<<" "<<temp_fsz<<" "<<temp_num_blk<<endl;
                        long long num_blk = stoll(temp_num_blk);
                        long long cur_add = stoll(temp_add);
                        long long cur_sz = stoll(temp_fsz);
                        long long last_blk_sz = cur_sz % (BLOCK_SIZE - PTR_SZ);
                        // cout<<cur_add<<endl;
                        if(num_blk == 1)
                        {
                            disk.seekg(cur_add);
                            char block_data[BLOCK_SIZE];
                            disk.read(block_data, BLOCK_SIZE);
                            printf("%sThe content of the file is:\n", GREEN);
                            string data_block = replace(block_data);
                            string my_data = replace(block_data);
                            printf("%s\n", my_data.c_str());
                        }
                        else
                        {
                            long long count = 0;
                            string res="";
                            //cout<<num_blk<<" "<<last_blk_sz<<endl;
                            while(count < num_blk)
                            {
                                //cout<<"Reading at: "<<cur_add<<endl;
                                disk.seekg(cur_add);
                                char temp_data[BLOCK_SIZE - PTR_SZ];
                                string dat="";
                                if(count == num_blk-1)
                                {//cout<<"count:"<<count<<endl;
                                    disk.read(temp_data, last_blk_sz);
                                    dat = temp_data;
                                    dat = dat.substr(0,last_blk_sz);
                                }
                                else
                                {
                                    disk.read(temp_data, BLOCK_SIZE - PTR_SZ);
                                    dat = temp_data;
                                }
                                res += dat;
                                //cout<<"Data: "<<dat<<endl;
                                if(count < num_blk - 1)
                                {
                                    char temp_add[PTR_SZ];
                                    disk.seekg(cur_add + BLOCK_SIZE - PTR_SZ);
                                    disk.read(temp_add, PTR_SZ);
                                    // cout<<"Next Add: "<<temp_add<<endl;
                                    string my_add = temp_add;
                                    my_add = my_add.substr(0,my_add.find_first_of('$'));
                                    // cout<<"here"<<endl;
                                    cur_add = stoll(my_add);
                                }
                                count++;
                            }
                            res = replace(res);
                            //cout<<res.size();
                            printf("%s%s", GREEN, res.c_str());
                        }
                    }
                    else if(ch2 == 4)  //Write
                    {
                        long long cur_fd;
                        printf("%sEnter file descriptor: ",WHITE);
                        cin>>cur_fd;
                        if(file_descr.find(cur_fd)==file_descr.end())
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second != 1 && file_descr[cur_fd].second >= 0)
                        {
                            printf("%sFile already opened in other mode!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second < 0)
                        {
                            printf("%sFile not opened!\n", RED);
                            continue;
                        }
                        string fname = file_descr[cur_fd].first;
                        long long inode_add = file_dets[fname].first;
                        printf("%sEnter file content: ",WHITE);
                        string content = "";
                        string temp_line = "";
                        while(temp_line != "end")
                        {
                            content += temp_line + '$';
                            cin>>temp_line;
                        }
                        content = content.substr(1);
                        // cout<<content<<endl;
                        long long temp_sz = content.size();
                        // string fname = file_descr[cur_fd].first;
                        // long long inode_add = file_dets[fname].first;
                        // cout<<inode_add<<endl;
                        disk.seekg(inode_add);
                        char temp[INODE_SIZE];
                        disk.read(temp, INODE_SIZE);
                        //cout<<temp<<endl;
                        string inode_data = temp;
                        inode_data = inode_data.substr(FNAME_SZ);
                        string temp_add = inode_data.substr(0, PTR_SZ);
                        inode_data.substr(PTR_SZ);
                        temp_add = temp_add.substr(0,temp_add.find_first_of('$'));
                        //cout<<"temp_add: "<<temp_add<<endl;
                        long long add = stoll(temp_add);
                        long long orig_add = add;
                        if(temp_sz <= BLOCK_SIZE)
                        {
                            disk.seekp(add);
                            disk.write(content.c_str(), content.size());
                        }
                        else
                        {
                            //cout<<"Size of content: "<<temp_sz<<endl;
                            long long cur_blocks =  temp_sz / (BLOCK_SIZE - PTR_SZ);
                            if(temp_sz % (BLOCK_SIZE - PTR_SZ) > 0)
                                cur_blocks++;
                            rem_blocks += cur_blocks - 1;
                            content.resize(cur_blocks * BLOCK_SIZE, '$');
                            //cout<<"Required blocks: "<<cur_blocks<<endl;
                            if(rem_blocks < cur_blocks - 1)
                            {
                                printf("%sEnough space is not available\n", RED);
                                continue;
                            }
                            // //cout<<add<<endl;
                            disk.seekp(add);
                            string cont = content.substr(0, BLOCK_SIZE - PTR_SZ);
                            disk.write(cont.c_str(), BLOCK_SIZE - PTR_SZ);
                            content = content.substr(BLOCK_SIZE - PTR_SZ);
                            long long cur_count = 0;
                            cur_blocks--;
                            for(long long i=0; i<block_bitmap.size() && cur_count < cur_blocks ; i++)
                            {   
                                if(block_bitmap[i] == '0')
                                {
                                    file_blocks[fname].push_back(i);
                                    block_bitmap[i] = '1';
                                    long long new_add = DATA_OFFSET + i * BLOCK_SIZE;
                                    // //cout<<new_add<<endl;
                                    string new_add_s = to_string(new_add);
                                    // //cout<<"New Add: "<<new_add_s<<endl;
                                    add = add + BLOCK_SIZE - PTR_SZ;
                                    disk.seekp(add);
                                    //cout<<"Write new_add at: "<<add<<endl;
                                    disk.write(new_add_s.c_str(), new_add_s.size());
                                    disk.seekp(new_add);
                                    string new_cont = "";
                                    cur_count++;
                                    new_cont = content.substr(0, BLOCK_SIZE - PTR_SZ);
                                    content = content.substr(BLOCK_SIZE - PTR_SZ);
                                    //printf("%sWriting at %lli\n%s\n",RED,new_add, new_cont.c_str());
                                    disk.write(new_cont.c_str(), BLOCK_SIZE - PTR_SZ);
                                    add = new_add;
                                }
                            }
                            disk.seekp(file_dets[fname].first);
                            string upd = get_inode(fname, orig_add, temp_sz, cur_blocks+1);
                            disk.write(upd.c_str(), upd.size());
                        }
                    }
                    else if(ch2 == 5)  //Append
                    {
                        long long cur_fd;
                        printf("%sEnter file descriptor: ",WHITE);
                        cin>>cur_fd;
                        if(file_descr.find(cur_fd)==file_descr.end())
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second != 2 && file_descr[cur_fd].second >= 0)
                        {
                            printf("%sFile already opened in other mode!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second < 0)
                        {
                            printf("%sFile not opened!\n", RED);
                            continue;
                        }
                        string fname = file_descr[cur_fd].first;
                        long long inode_add = file_dets[fname].first;
                        disk.seekg(inode_add);
                        // //cout<<"Inode address: "<<inode_add<<endl;
                        char temp[INODE_SIZE];
                        disk.read(temp,INODE_SIZE);
                        string temp_str = temp;
                        struct inode new_node = get_details(temp_str);
                        printf("%sEnter file content: ",WHITE);
                        string content = "";
                        string temp_line = "";
                        while(temp_line != "end")
                        {
                            content += temp_line + '$';
                            cin>>temp_line;
                        }
                        content = content.substr(1);
                        printf("%s%s",RED, content.c_str());
                        long long temp_sz = content.size();
                        long long total_size = temp_sz + new_node.size;
                        long long cur_blocks =  total_size / (BLOCK_SIZE - PTR_SZ);
                        if(total_size % (BLOCK_SIZE - PTR_SZ) > 0)
                            cur_blocks++;
                        //cout<<"Cur_blks: "<<new_node.blocks<<"\nNew_blks: "<<cur_blocks - new_node.blocks<<endl;
                        if(rem_blocks < cur_blocks - new_node.blocks)
                        {
                            printf("%sNot enough space to write data\n", RED);
                            continue;
                        }
                        long long last_blk_sz = new_node.size % (BLOCK_SIZE - PTR_SZ);
                        //cout<<last_blk_sz<<endl;
                        long long next_add = new_node.add;
                        long long count = 0;
                        while(count < new_node.blocks - 1)
                        {
                            //cout<<"Next addr: "<<next_add<<endl;
                            disk.seekg(next_add + BLOCK_SIZE - PTR_SZ);
                            char addr[PTR_SZ];
                            disk.read(addr, PTR_SZ);
                            string cur_addr = addr;
                            cur_addr = cur_addr.substr(0, cur_addr.find_first_of('$'));
                            next_add = stoll(cur_addr);
                            count++;
                        }
                        //cout<<"Outside"<<endl;
                        disk.seekg(next_add + last_blk_sz);
                        long long rem_sz = BLOCK_SIZE - last_blk_sz - PTR_SZ;
                        if(content.size() < rem_sz)
                        {
                            disk.write(content.c_str(), content.size());
                        }
                        else
                        {
                            long long new_blocks = cur_blocks - new_node.blocks;
                            long long add = next_add;
                            disk.write(content.substr(0,rem_sz).c_str(), rem_sz);
                            long long my_count = 0;
                            rem_blocks += new_blocks;
                            for(long long i=0; i< block_bitmap.size() && my_count < new_blocks; i++)
                            {
                                if(block_bitmap[i] == '0')
                                {
                                    file_blocks[fname].push_back(i);
                                    block_bitmap[i] = '1';
                                    long long new_add = DATA_OFFSET + i * BLOCK_SIZE;
                                    //cout<<new_add<<endl;
                                    string new_add_s = to_string(new_add);
                                    //cout<<"New Add: "<<new_add_s<<endl;
                                    add = add + BLOCK_SIZE - PTR_SZ;
                                    disk.seekp(add);
                                    disk.write(new_add_s.c_str(), new_add_s.size());
                                    disk.seekp(new_add);
                                    string new_cont = "";
                                    my_count++;
                                    if(my_count < new_blocks -1)
                                    {
                                        new_cont = content.substr(0, BLOCK_SIZE - PTR_SZ);
                                        content = content.substr(BLOCK_SIZE - PTR_SZ);
                                    }
                                    else
                                        new_cont = content;
                                    disk.write(new_cont.c_str(), new_cont.size());
                                    add = new_add;
                                }
                            }
                            disk.seekp(file_dets[fname].first);
                            long long tot_blocks = total_size / (BLOCK_SIZE - PTR_SZ);
                            if(total_size % (BLOCK_SIZE - PTR_SZ) > 0)
                                tot_blocks++;
                            string upd = get_inode(fname, new_node.add, total_size, tot_blocks);
                            disk.write(upd.c_str(), upd.size());
                        }
                    }
                    else if(ch2 == 6)  //Close
                    {
                        long long cur_fd;
                        printf("%sEnter file descriptor: ",WHITE);
                        cin>>cur_fd;
                        if(file_descr.find(cur_fd)==file_descr.end())
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second == -2)
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second == -1)
                        {
                            printf("%sFile not opened in any mode!\n", RED);
                            continue;
                        }
                        string fname = file_descr[cur_fd].first;
                        // //cout<<"Closing file "<<fname<<endl;
                        open_files.erase(fname);
                        file_descr[cur_fd].second = -2;
                        printf("%sFile closed successfully\n", GREEN);

                    }
                    else if(ch2 == 7)   //Delete
                    {
                        long long cur_fd;
                        printf("%sEnter file descriptor: ",WHITE);
                        cin>>cur_fd;
                        if(file_descr.find(cur_fd)==file_descr.end())
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second == -2)
                        {
                            printf("%sInvalid file descriptor!\n", RED);
                            continue;
                        }
                        if(file_descr[cur_fd].second == -1)
                        {
                            printf("%sFile not opened in any mode!\n", RED);
                            continue;
                        }
                        string fname = file_descr[cur_fd].first;
                        open_files.erase(fname);
                        file_descr.erase(cur_fd);
                        long long inode_add = file_dets[fname].first;
                        long long in_ind = inode_add;
                        long long off = inode_add % BLOCK_SIZE;
                        long long ind_no = off % INODE_SIZE;
                        inode_bitmap[ind_no] = '0';
                        for(long long x:file_blocks[fname])
                            block_bitmap[x] = '0';
                        printf("File deleted successfully");
                    }
                    else if(ch2 == 8) //List of files
                    {
                        if(file_dets.size() == 0)
                            printf("%sNo files exist\n", GREEN);
                        else
                        {
                            printf("%sBelow are the list of files\n", GREEN);
                            for(auto file = file_dets.begin();file!=file_dets.end(); file++)
                                printf("%s%s\n", GREEN, file->first.c_str());
                        }
                    }
                    else if(ch2 == 9)
                    {
                        if(open_files.size() == 0)
                            printf("%sNo files are opened\n", GREEN);
                        else
                        {
                            printf("%sBelow are the list of files\n", GREEN);
                            for(auto file = open_files.begin();file != open_files.end(); file++)
                                printf("%s%s\t%lli\t%s\n", GREEN, file->first.c_str(), file->second.first, mode_map[file->second.second].c_str());
                        }
                    }
                    else if(ch2 == 10)
                    {
                        disk.seekp(IMAP_OFFSET);
                        string bitmap = inode_bitmap + block_bitmap;
                        disk.write(bitmap.c_str(), bitmap.size());
                        fd = 0;
                        file_descr.clear();
                        file_dets.clear();
                        open_files.clear();
                        inode_bitmap.clear();
                        block_bitmap.clear();
                        // file_blocks.clear();
                        disk.close();
                        printf("%s%s unmounted successfully\n", GREEN, dname.substr(0,dname.find_first_of('.')).c_str());
                        break;
                    }
                }
            }
        }
        else if(ch == 3)
        {
            printf("%sExiting the application\n", GREEN);
            break;
        }
        else
        {
            printf("%sInvalid Choice!\n", RED);
        }
    }
    return 0;
}