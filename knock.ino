int mic_analog = 0;
int current_sum = 0;
int current_average = 0;
int count = 0;
int knock_variance = 0;
int MIC_PIN = 2; //Analog input pin
int BUTTON_PIN = 7; //Button input pin
int CYCLES_TO_AVERAGE = 20; //Noise threshold averaging time
int KNOCK_THRESHOLD = 5; //How far above threshold to trigger knock
boolean KNOCKING = 0;
int kv_data[10]; //Soft cap of 10 knocks in a sequence
int kv_data_size = 10;
int kv_index = 0;
int kv_preav;
int MAX_KV_TIME = 200;
int MAX_KV_COUNTER = 0;
int kv_sep = 200; //Knock minimum separation
int kv_sep_ct = 0;
boolean has_knocked = 0;
int kv_ksep = 0;
int kv_ksep_max = 700; //Knock sequence timeout
int kv_seq_data[10];
int kv_seq_size = 10;
int kv_seq_index = 0;
boolean RECORDING = 0;
int LOCK_SEQ[10];
int LOCK_VARIANCE = 150; //TWEAK TO PREFERENCE

void setup() {
  pinMode(7, INPUT); //Record sequence button
  pinMode(8, OUTPUT); //Result of successful recognition
  Serial.begin(115200);
  Serial.println("============");
  Serial.println("Initialized.");
  Serial.println("============");
  Serial.flush();
}

//Get the max value from an array
int arr_max(int arr[]){
  int m = 0;
  for(int i = 0; i <kv_data_size; i++){
    if(arr[i] > m){
      m = arr[i];
    }
  }
  return m;
}

//Debug function
void printAverage(){
    Serial.print(count);
    Serial.print(" cycle average: ");
    Serial.println(current_average);
}

//Update noise baseline
void updateAverage(int amount){
  if(count >= CYCLES_TO_AVERAGE){
    current_average = current_sum/count;
    //printAverage();
    count = 0;
    current_sum = 0;
  }
    current_sum += amount;
    count++;
}

//Print an array to the console
void arr_print(int arr[], int size){
  for(int i = 0; i < size; i++){
    if(arr[i] > 0){
      Serial.print(arr[i]);
      Serial.print(" ");
    }
  }
  Serial.println();
}

//End a knock
void kv_end(){
  KNOCKING = 0;
  kv_index = 0;
  MAX_KV_COUNTER = 0;
  kv_sep_ct = 0;
  kv_ksep = 0;
  //print the largest value of the knock period
  Serial.println(arr_max(kv_data));
  //reset array
  for(int i = 0; i < kv_data_size; i++){
    kv_data[i] = 0;
  }
}

//Get current knock data
void kv_collect(int kv){
  if(KNOCKING == 1){
    if(kv_index < kv_data_size){

      kv_data[kv_index] = kv;
      kv_index++;
    }
  }
}

//Add a knock to the current sequence
void kv_seq_collect(int seq){
  if(RECORDING == 1){
   LOCK_SEQ[kv_seq_index] = seq;
  }
  if(kv_seq_index < kv_seq_size){
    kv_seq_data[kv_seq_index] = seq;
    kv_seq_index++;
  } else {
    Serial.println("Error - sequence max reached.");
  }
}

//Register a knock
void kv_begin(){
  //new K sequence
    KNOCKING = 1;
    kv_preav = current_average;
    kv_sep_ct = 0;

    //Add knock to sequence if sequence is continuing
    if(kv_ksep <= kv_ksep_max){
      int j = kv_ksep;
      kv_seq_collect(j);
    }   
    has_knocked = 1;
}

//Called when a sequence is valid.
void unlock(){
  Serial.println("VALID SEQUENCE.");
  digitalWrite(8, HIGH);
  delay(300);
  digitalWrite(8, LOW);
  delay(2000);
  Serial.println("Ready for usage.");
}

void validate_sequence(){
  boolean valid = 1;
  for(int i = 0; i < kv_seq_size; i++){
    Serial.print("VARIANCE: ");
    Serial.println(LOCK_SEQ[i] - kv_seq_data[i]);
    if(abs(LOCK_SEQ[i] - kv_seq_data[i]) > LOCK_VARIANCE){
      valid = 0;
    }
  }
  if(valid == 1){
    unlock();
  }
}

//Resets the master sequence
void resetLock(){
  for(int i = 0; i < 10; i++){
    LOCK_SEQ[i] = 0;
  }
}

//Ends a knock sequence
void kv_seq_end(){
  kv_ksep = 0;
  arr_print(kv_seq_data, kv_seq_size);
  Serial.println("===PATTERN ENDED===");

  if(RECORDING == 1){
    if(kv_seq_index < 2){
      Serial.println("Lock code too short.");
      resetLock();
    } else {
      Serial.println("New lock pattern recorded.");
      Serial.print("Length :");
      Serial.println(kv_seq_index);
      arr_print(LOCK_SEQ, 10);
    }
    RECORDING = 0;
  }

  validate_sequence();

  //Reset sequence
  has_knocked = 0;
  kv_seq_index=0;
  for(int i = 0; i < kv_seq_size; i++){
    kv_seq_data[i] = 0;
  }
}


void loop() {
  mic_analog = analogRead(MIC_PIN);
  updateAverage(mic_analog);
  knock_variance = mic_analog - current_average;

  //Listen for knock
  //For a knock to register, it must be above the threshold, a knock must not already be in progress, and there must be separation between knocks.
  if(knock_variance > KNOCK_THRESHOLD && current_average > 0 && KNOCKING == 0 && kv_sep_ct > kv_sep){
    kv_begin();
    kv_collect(knock_variance);
  } else {
    if(KNOCKING == 1){
      kv_collect(knock_variance);
      MAX_KV_COUNTER++; 
    } else {
      kv_sep_ct++;
      if(kv_ksep > kv_ksep_max && has_knocked == 1){
        kv_seq_end();
      }
    }
    if((KNOCKING == 1 && mic_analog - kv_preav < KNOCK_THRESHOLD) || MAX_KV_COUNTER >= MAX_KV_TIME){
      kv_end();
    }    
  }
  kv_ksep++;

  if(digitalRead(BUTTON_PIN) == HIGH && RECORDING == 0){
    Serial.println("Waiting to record new sequence.");
    resetLock();
    RECORDING = 1;
    delay(500);
  }
  delay(1);
}
