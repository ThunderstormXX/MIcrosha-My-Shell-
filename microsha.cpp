#include <unistd.h>     // getpid(), getcwd()
#include <sys/types.h>  // type definitions, e.g., pid_t
#include <sys/wait.h>   // wait()
#include <signal.h>     // signal name constants and kill()
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include <sys/stat.h> //for open/close
#include <fcntl.h> // for open/close
#include <utility>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include "test.cpp"

using namespace std;
//=====================================================//
const vector< char> Sigma {
    'q','w','e','r','t','y','u','i','o','p','a','s','d','f','g','h','j','k','l','z','x',
    'c','v','b','n','m','.','Q','W','E','R','T','Y','U','I','O','P','A','S','D','F','G','H','J','K','L','Z','X',
    'C','V','B','N','M' ,'0','1','2','3','4','5','6','7','8','9'
};
const vector< char> Sigma2 {
    'q','w','e','r','t','y','u','i','o','p','[',
    ']','a','s','d','f','g','h','j','k','l',';','z','x',
    'c','v','b','n','m','.','Q','W','E','R','T','Y','U','I','O','P','{',
    '}','A','S','D','F','G','H','J','K','L',':','Z','X',
    'C','V','B','N','M' ,'0','1','2','3','4','5','6','7','8','9','@','#','$','%','^'
};



//=====================================================//
int lsh_cd(char** args);
int lsh_exit(char** args);
int lsh_ls(char** args);
//=====================================================//
char* builtin_str[] = {
  "cd",
  "exit",
  "ls",
};
//=====================================================//
int (*builtin_func[]) (char**) = {
  &lsh_cd,
  &lsh_exit,
  &lsh_ls,
};
//=====================================================//
int lsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}
//=====================================================//
bool suit_strings(string a, string b) {
	//*text
  
  a = a.substr(1);//шаблон или папка
  b = b.substr(1);//папка
  if (a.find('*', 0) != a.npos){
	a = a.substr(1);
	int la = a.size();
	int lb = b.size();
	if (lb < la) return 0;
	for (int i = 0; i < la; i++){
		if (a[la - 1 - i] != b[lb - 1 - i] && a[la - 1 - i] != '?') return 0;
	}
	return 1;
  }
  else{
    if (a.size() != b.size()) return 0;
	  for (int i = 0; i < a.size(); i++) {
		  if (a[i] != b[i] && a[i] != '?') {
			  return 0;
		  }
	  }
  }
  //?
	return 1;
}
//=====================================================//
//ищем стартовые папки
void Initial_Folders(vector<string>& path, vector<string>& ans) {
	if (path[0] == "/*") {
		DIR* dir;
		struct dirent* de;
		dir = opendir("/");
		while (dir) {
			de = readdir(dir);
			if (!de) break;
			if (de->d_name[0] != '.' && de->d_type == 4) {
				ans.push_back('/' + (string)de->d_name);
			}
		}
		closedir(dir);
		path.erase(path.begin());
	}
	else {
		if (path[0].find('?', 0) == path[0].npos && path[0].find('*', 0) == path[0].npos) {
			ans.push_back(path[0]);
			path.erase(path.begin());
		}
		else {
			DIR* dir;
			struct dirent* de;
			dir = opendir("/");
			while (dir) {
				de = readdir(dir);
				if (!de) break;
				if (de->d_name[0] != '.' && de->d_type == 4
					&& suit_strings(path[0], '/' + (string)de->d_name)) {
					ans.push_back('/' + (string)de->d_name);
				}
			}
			closedir(dir);

			path.erase(path.begin());
		}
	}
}
//=====================================================//

string pumping( const string &str ) {
    string temp;
	//cout << str  << '\n';
    for(int i = 0 ; i < str.size() ; ++i) {
        if(str[i] == '*') {
            temp += "(";
            for(int i = 0 ; i < Sigma.size() - 1 ; ++i)  temp = temp + Sigma[i] + "|";
            temp+=Sigma[Sigma.size()-1];
            temp += ")*";
        } else if(str[i] == '?') {
            temp += "(";
            for(int i = 0 ; i < Sigma.size() - 1 ; ++i)  temp = temp + Sigma[i] + "|";
            temp+=Sigma[Sigma.size()-1];
            temp += ")";
        } else {
            temp += str[i];
        }
    }
	return temp;
}


bool checking (const string & str, const string & path) {
    re_dfa_type dfa;
    bool ok = dfa.compile (str.cbegin (), str.cend ());
    int current = 0;
    for(int i = 0 ; i < path.size() ; ++i) {
        auto k = dfa.m_transition[current];
        //cout << "[" << current << "-->" <<k[(int)path[i]] << "]" << '\n'; 
        current = k[(int)path[i]];
    }
    if((dfa.m_transition[current])[-1] == -1) return 1;
    return 0;
}

//=====================================================//


vector<string> summ_massive(const vector<string> &str1 ,const vector<string> &str2) {
  vector<string> res ;
  for(int i = 0; i < str1.size() ; ++i) {
    res.push_back(str1[i]);
  }
  for(int i = 0; i < str2.size() ; ++i) {
    res.push_back(str2[i]);
  }
  return res ;
}

vector<string> path_to_mass(const string& path ,const string& regex ) {
    //if( path == "") path = "/";
    
    vector<string> res;
    DIR *dir;
    struct dirent *de;
    
    dir = opendir( path.c_str()); /*your directory*/
    //cout << "1";
	while(dir) {
        //cout << '\n' << path;
        de = readdir(dir);
        if (!de) break;
        cout << "[" << de->d_name << "]\n";
		//cout << regex << '\n';
		if(de->d_name[0] != '.' && checking(regex ,de->d_name)) {
          if(path == "/") {;
		    res.push_back(path +(string)de->d_name);
			//cout << 1;
		  } else {
            res.push_back(path + "/" +(string)de->d_name);
			//cout << 2;
		  }
        }
    }
    closedir(dir);
    for(auto k : res ) cout <<"_" << k << "_"; 
	return res;
}

vector<string> dir_path(vector<string>& mass) {
	//cout << "mass: " ;
	//cout << mass.size();
	//for(auto k : mass) cout <<"_" << k << "_";
	vector<string> mass_path ;
	// if(!mass.size()) {
	// 	mass_path.push_back(".");
	// 	return mass_path;
	// }
	if(mass[0] == "/") {
		mass_path.push_back("/");
	} else  {
		mass_path.push_back(".");
	}

	for(int i = 0 ; i < mass.size() ; ++i) {
		vector<string> mass_path_temp;
		vector<string> temp;
		string regex = pumping(mass[i]);
			//cout << mass_path.size() << " ";
		
		if( mass[i] != "/" && mass[i] != ".") {
			for(int j = 0 ; j < mass_path.size() ;++j) {

				temp = path_to_mass(mass_path[j] , regex);

				mass_path_temp = summ_massive(mass_path_temp , temp);
				//cout << temp.size() << " ";
				//cout << mass_path_temp.size() << " ";
			}
			mass_path = mass_path_temp;
		}
	//cout << mass_path.size();
    }

	//for(int i = 0 ; i < mass_path.size() ; ++i) cout << mass_path[i]  << '\n';
	
  return mass_path;
	
}
//=====================================================//
vector<string> reformat_args(char** args) {
	vector<string> ans;
	while (*args != NULL) {
		ans.push_back(*args);
		args++;
	}
	return ans;
}
//=====================================================//
vector<string> Tokenizator(string a) {
	vector<string> tokens;
	string temp;
	char c;
	while (!a.empty()) {
		if (a.rfind("/") != a.npos) {
			auto it = a.rfind("/");
			temp = a.substr(it + 1);
			if (!temp.empty()) tokens.push_back(temp);
			tokens.push_back("/");

			a = a.substr(0, it);
		}
		else {
			tokens.push_back(a);
			break;
		}
	}

	reverse(tokens.begin(), tokens.end());
	return tokens;
}
//=====================================================//
int lsh_ls(char** args) {

	vector<string> argums = reformat_args(args);
		for(auto k : argums ) cout << k << " ";

	if (argums.size() == 1 || argums.back()[0] == '-') {
			vector<char*> args0; 
			for (size_t i = 0; i < argums.size(); ++i) {
				args0.push_back((char*)argums[i].c_str());
			}
			args0.push_back(nullptr);
			pid_t pid;
			pid = fork();
			if (pid == 0) {
				execvp(args0[0], &args0[0]);
				perror(args0[0]);
				exit(0);
			}
			else if (pid > 0) {
				wait(0);
			}

		return 1;
	}
	else {
		vector<string> folders = Tokenizator(argums.back());
		vector<string> ans;
				ans = dir_path(folders);

		//for(auto k : ans ) cout << k << " ";
		///system("pause");
		// if( folders.empty()) {
		// 	vector<char* > args0;
		// 	for (size_t i = 0; i < argums.size(); ++i) {
		// 		args0.push_back((char*)argums[i].c_str());
		// 	}
		// 	args0.push_back(nullptr);
		// 	pid_t pid;
		// 	pid = fork();
		// 	if (pid == 0) {
		// 		execvp(args0[0], &args0[0]);
		// 		perror(args0[0]);
		// 		exit(0);
		// 	}
		// 	else if (pid > 0) {
		// 		wait(0);
		// 	}
		// } else {
			for (int i = 0; i < ans.size(); i++) {
				argums.back() = ans[i];
				//cout << argums.back() << ':' << endl;
				vector<char* > args0;
				for (size_t i = 0; i < argums.size(); ++i) {
					args0.push_back((char*)argums[i].c_str());
				}
				args0.push_back(nullptr);
				pid_t pid;
				pid = fork();
				if (pid == 0) {
					execvp(args0[0], &args0[0]);
					perror(args0[0]);
					exit(0);
				}
				else if (pid > 0) {
					wait(0);
					continue;
				}
			}
		//}
	}
	return 1;
}
//=====================================================//
int lsh_cd(char** args) {
	if (args[1] == NULL) {
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	}
	else {
		if (chdir(args[1]) != 0) {
			perror("lsh");
		}
	}
	return 1;
}
//=====================================================//
int lsh_exit(char** args) {
	exit(0);
	return 0;
}

//=====================================================//
pair<char**, int> parse_line(char* line) {
	int i = 0;
    string buf; 
    stringstream ss(line);
    vector<char*> tokens;
    while (ss >> buf){
	char *temp = new char[buf.length() + 1];
        strcpy(temp,buf.c_str());
        tokens.push_back(temp);
    }
    
    char** argv = new char*[tokens.size() + 1];
    for ( int k = 0; k < tokens.size(); k++ ){
      	argv[k] = tokens[k];
    }
	argv[tokens.size()] = NULL;
	return {argv, tokens.size()};
}
//=====================================================//
void multipipe(char *cmd,char* args[], int argnum, int& Stop){
    int pipes = 0;
    for(int i = 0; i < argnum; i++){
        if(strcmp(args[i], "|") == 0) {pipes = 1; }
    }
    
	if(pipes) {
        for(int i = 0; i < argnum; i++){
                if(strcmp(args[i], "|") == 0){
                        args[i] = NULL;
                        char* right[argnum - i];
                        int c = 0;
                        for(int j = i; j < argnum - 1; j++){
                                right[j - i] = args[j + 1];
                                c++;
                        }
                        right[c] = NULL;
                        int p[2];
                        pipe(p);
                        pid_t lpid = fork();
                        if(lpid == 0) {//left (child)
                            dup2(2, 1);
                            dup2(p[1], STDOUT_FILENO);
                            execvp(cmd, args);
                        }
                        else {//right (parent)
                            pid_t rpid = fork();
                            close(p[1]);
                            if(rpid == 0) { //right child
                                dup2(2, 1);
                                dup2(p[0], STDIN_FILENO);
                                multipipe(right[0], right, c, Stop);
                                close(p[1]);
                            } else { //parent
								waitpid(rpid, 0, 0);
							}
						}
                        break;
                }
        }
    }
    else {
        if(strcmp(cmd, "cd") == 0){ //built-in command cd
            if(args[1] == NULL){
                fprintf(stderr, "lsh: expected argument to \"cd\"\n");
            }
            else{
                chdir(args[1]);
            }
        }
		else 
		if(strcmp(cmd, "ls") == 0) { //ls
            lsh_ls(args);
        }
        else{ //standard forking process
            pid_t childID = fork();
            if(childID < 0){
                perror("Error when forked");  
				Stop = 1;               
            }
            else 
			if(childID == 0){ //Child process
                        for(int i = 0; i < argnum; i++){
                                if(strcmp(args[i], ">") == 0){
                                        int newstdout = open(args[i + 1], O_WRONLY | O_CREAT , S_IRWXU | S_IRWXG | S_IRWXO);
                                        close(1);
                                        dup(newstdout);
                                        close(newstdout);
                                        args[i] = NULL;
                                }
                                else if(strcmp(args[i], "<") == 0){
                                        int newstdin = open(args[i + 1], O_RDONLY);
                                        close(0);
                                        dup(newstdin);
                                        close(newstdin);
                                        args[i] = NULL;
                                }
                        }
                        execvp(cmd, args);
                        perror(cmd); //only happens when there is an error
						Stop = 1;
            }
			else{
				if (waitpid(childID, 0, 0) < 0) {//parent process
          			Stop = 1;
	  				perror("Error when waiting for child");
        		}
      	  	}
            
        }
    }
	Stop = 1;
}
//=====================================================//
void Execution(char* cmd, char *args[], int argnum, int& Stop){
	int pipes = 0;
    for(int i = 0; i < argnum; i++){
		if(strcmp(args[i], "|") == 0) { pipes = 1; }
    }

	if (pipes) {
		for(int i = 0; i < argnum; i++) {
			if(strcmp(args[i], "|") == 0) {
               	args[i] = NULL;
                char* right[argnum - i];
                int c = 0;
                for(int j = i; j < argnum - 1; j++) {
	                right[j - i] = args[j + 1];
                    c++;
                }
                right[c] = NULL;
                int p[2];
                pipe(p);
                pid_t lpid = fork();
                if(lpid == 0){//left (child)
					dup2(2,1);
                    dup2(p[1], STDOUT_FILENO);
                    execvp(cmd, args);
                }
                else {//right (parent)
					pid_t rpid = fork();
					close(p[1]);
					if(rpid == 0){ //right child
						dup2(2,1);
                    	dup2(p[0], STDIN_FILENO);
                    	multipipe(right[0], right, c, Stop);
						close(p[1]);
					} else { //parent
						waitpid(rpid, 0, 0);
					}
                }
                break;
			}
		}
    }
	else{
        if(strcmp(cmd, "cd") == 0) { //cd
            if(args[1] == NULL){
                fprintf(stderr, "lsh: expected argument to \"cd\"\n");
            }
            else{
                chdir(args[1]);
            }
        }
		else 
		if(strcmp(cmd, "ls") == 0) { //ls
            lsh_ls(args);
        }
        else { //fork
      		pid_t childID = fork();
      		if(childID < 0) {
        		perror("Error when forked");
				Stop = 1;
      		}
      		else 
			if(childID == 0) { //Child process
                for(int i = 0; i < argnum; i++) {
                    if(strcmp(args[i], ">") == 0) {
                        int newstdout = open(args[i + 1], O_WRONLY | O_CREAT , S_IRWXU | S_IRWXG | S_IRWXO);
                        close(1);
                        dup(newstdout);
                        close(newstdout);
                        args[i] = NULL;
                    }
                    else if(strcmp(args[i], "<") == 0){
                        int newstdin = open(args[i+1], O_RDONLY);
                        close(0);
                    	dup(newstdin);
                    	close(newstdin);
                        args[i] = NULL;
                    }
                }
        		execvp(cmd, args);
        		perror(cmd); //only happens when there is an error
				Stop = 1;
      		}
			else{
				if (waitpid(childID, 0, 0) < 0) {//parent process
          			Stop = 1;
	  				perror("Error when waiting for child");
        		}
      	  	}
    	  }
     }
}
//=====================================================//
//главный цикл
void loop() {
	char* line;
	char** args;
	int CountArgs;
	int status;
	string Line;
	int Stop = 0;

	while(!Stop) {
		int p = 0;
		pid_t uid = getuid();
		if (p == 0) {
			char str[100];
			getcwd(str, 100 * sizeof(char));
			if (uid == 0) {
				printf("[%s] !", str);
			}
			else {
				printf("[%s] >", str);
			}
		}
		if(!getline(cin,Line) ){ Stop = 1; }
		char *line = new char[Line.length() + 1];
        strcpy(line,Line.c_str());
		if (Line.size() == 0) continue;
		pair<char**, int> Tokens = parse_line(line);
		args = Tokens.first;
		CountArgs = Tokens.second;
		//status = lsh_execute(args);

		Execution(args[0],args, CountArgs, Stop);

		free(line);
		free(args);
	}
}
//=====================================================//
int main(int argc, char** argv)
{
	loop();
	return 0;
}