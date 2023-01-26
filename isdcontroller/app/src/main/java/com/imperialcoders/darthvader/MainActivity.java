package com.imperialcoders.darthvader;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.BluetoothLeScanner;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements ISDControllerUIDelegate {


    int[] ctrlIds = {R.id.playImpMarchBtn,
            R.id.powerUpSeqBtn,
            R.id.garageChuteOnBtn,
            R.id.garageChuteOffBtn,
            R.id.powerUpMainBtn,
            R.id.powerOffMainBtn,
            R.id.powerUpMainEngBtn,
            R.id.powerOffMainEngBtn,
            R.id.powerUpMiniEngBtn,
            R.id.powerOffMiniEngBtn,
            R.id.audioLabel,
            R.id.audioVol1,
            R.id.playImpMarchVaderBtn};

    Handler handler;

    ISDBluetoothController btController;



    @Override
    public void deviceReady(BluetoothDevice device) {

    }

    @Override
    public void adapterEnabled(BluetoothAdapter adapter) {
        TextView tv = findViewById(R.id.statusTxt);
        tv.setText("BT BLE Adapter enabled");
    }

    @Override
    public void scanStarted() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("BLE Adapter is scanning...");


    }

    @Override
    public void onScan(String deviceName) {
        TextView tv = findViewById(R.id.statusTxt);
        String s = deviceName;
        if (null == s) {
            s = "N/A";
        }
        else {
            s = s.trim();
        }

        tv.setText("searching, found ... " + s);
    }

    @Override
    public void scanStopped() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("BLE Adapter scanning stopped");
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView stat = findViewById(R.id.statusTxt);


        Button rescanBtn = findViewById(R.id.rescanBtn);
        rescanBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.rescanBtnClicked();
            }
        });


        Button disconnectBtn = findViewById(R.id.disconnectBtn);
        disconnectBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.disconnectBtnClicked();
            }
        });

        Button connectBtn = findViewById(R.id.connectBtn);
        connectBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.connectBtnClicked();
            }
        });

        SeekBar audioVol = findViewById(R.id.audioVol1);

        audioVol.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                MainActivity.this.audioVol1Changed(seekBar);
            }
        });



        Button btn = findViewById(R.id.playImpMarchBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPlayImpMarchBtnClicked();
            }
        });

        btn = findViewById(R.id.powerUpSeqBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerUpSeqBtnClicked();
            }
        });

        btn = findViewById(R.id.garageChuteOnBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onGarageChuteOnBtnClicked();
            }
        });

        btn = findViewById(R.id.garageChuteOffBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onGarageChuteOffBtnClicked();
            }
        });

        btn = findViewById(R.id.powerUpMainBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerUpMainBtnClicked();
            }
        });

        btn = findViewById(R.id.powerOffMainBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerOffMainBtnClicked();
            }
        });

        btn = findViewById(R.id.powerUpMainEngBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerUpMainEngBtnClicked();
            }
        });

        btn = findViewById(R.id.powerOffMainEngBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerOffMainEngBtnClicked();
            }
        });

        btn = findViewById(R.id.powerUpMiniEngBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerUpMiniEngBtnClicked();
            }
        });

        btn = findViewById(R.id.powerOffMiniEngBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onPowerOffMiniEngBtnClicked();
            }
        });

        btn = findViewById(R.id.enginesStartUpSeqBtn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity.this.onEnginesStartUpSeqBtnClicked();
            }
        });


        btController = new ISDBluetoothController();
        btController.init(this, this);

        enableControls(false);
    }

    private void onPowerOffMiniEngBtnClicked() {
        btController.setLightspeedEngines(false);
    }

    private void onPowerUpMiniEngBtnClicked() {
        btController.setLightspeedEngines(true);
    }

    private void onPowerOffMainEngBtnClicked() {
        btController.setMainEngines(false);
    }

    private void onPowerUpMainEngBtnClicked() {
        btController.setMainEngines(true);
    }

    private void onPowerOffMainBtnClicked() {
        btController.setMainSystemsPower(false);
    }

    private void onPowerUpMainBtnClicked() {
        btController.setMainSystemsPower(true);
    }

    private void onGarageChuteOffBtnClicked() {
        btController.setGarbageChute(false);
    }

    private void onGarageChuteOnBtnClicked() {
        btController.setGarbageChute(true);
    }

    private void onPowerUpSeqBtnClicked() {
        CheckBox cb = findViewById(R.id.useAudioWithLight);
        btController.startPowerSequence(cb.isChecked());
    }

    private void onEnginesStartUpSeqBtnClicked() {
        CheckBox cb = findViewById(R.id.useAudioWithLight);
        btController.startEnginesSequence(cb.isChecked());
    }

    private void onPlayImpMarchBtnClicked() {
        btController.playImperialMarch();
    }

    private void audioVol1Changed(SeekBar seekBar) {
        Log.i("sb-ui", "sb progress:" + seekBar.getProgress());
        btController.changeAudio1Volume(seekBar.getProgress(),0,100);
    }

    private void connectBtnClicked() {
        btController.connectToDevice();
    }

    private void disconnectBtnClicked() {
        btController.disconnectFromDevice();
    }

    private void rescanBtnClicked() {
        btController.rescan();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.i("perms","requestCode: " + requestCode);

        btController.onAppCompatRequestPermissionsResult(requestCode, permissions, grantResults);
    }


    void deviceReady() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("ISD device ready");

        enableControls(true);
    }

    public void deviceDisconnected() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("ISD device disconnected!");

        Button b = findViewById(R.id.connectBtn);
        b.setEnabled(true);

        b = findViewById(R.id.disconnectBtn);
        b.setEnabled(false);

        enableControls(false);
    }

    @Override
    public void deviceConnectionFailure() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("ISD device connection failure!");
    }

    @Override
    public void scanTimedOut() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("Scan timed out");
    }

    @Override
    public void scanFailed(BluetoothLeScanner scanner, int errorCode) {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("BLE Scan failed. Error code: " + errorCode);
    }

    public void deviceConnected() {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("ISD device connected!");

        Button b = findViewById(R.id.connectBtn);
        b.setEnabled(false);

        b = findViewById(R.id.disconnectBtn);
        b.setEnabled(true);

        enableControls(true);

    }

    public void deviceFound(BluetoothDevice device) {
        TextView stat = findViewById(R.id.statusTxt);
        stat.setText("ISD device found!");
    }


    void enableControls(boolean val) {
        for (int i:ctrlIds) {
            View v = findViewById(i);
            v.setEnabled(val);
        }
    }

}