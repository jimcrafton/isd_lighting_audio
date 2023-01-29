package com.imperialcoders.darthvader;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Handler;
import android.util.Log;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

interface ISDControllerUIDelegate {

    public void deviceReady(BluetoothDevice device);

    public void adapterEnabled(BluetoothAdapter adapter);

    public void scanStarted();

    public void onScan(String deviceName);

    public void scanStopped();

    public void deviceFound(BluetoothDevice device);

    public void deviceConnected();

    public void deviceDisconnected();

    public void deviceConnectionFailure();

    public void scanTimedOut();

    void scanFailed(BluetoothLeScanner scanner, int errorCode);
}


public class ISDBluetoothController {

   private ISDControllerUIDelegate delegate;

    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    BluetoothGatt btConnection;
    BluetoothGattCharacteristic btWriteCharacteristic;
    int writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE;
    BluetoothDevice btDevice;
    Context context;
    Handler handler;
    boolean permsGranted = false;
    boolean scanningInProgress = false;

    private int scanTimeout = 10;

    static final int RESCAN_DELAY = 300;

    static final int BT_PERMS_REQUEST = 101;
    private static final int REQUEST_ENABLE_BT = 1;
    //frame
    //ETX msg:uint8 arg:uint8 ETX
    static final byte STX = 0x01;
    static final byte ETX = 0x03;

    static final byte ARG_FALSE = 0x00;
    static final byte ARG_TRUE = 0x01;

    static final byte ARG_WITH_AUDIO = 0x10;


    static final byte MSG_UPDATE_AUDIO_VOLUME = (byte)0xFF;
    static final byte MSG_PLAY_IMPERIAL_MARCH = (byte)0xFE;
    static final byte MSG_GARBAGE_CHUTE_ON = (byte) 0xFD;
    static final byte MSG_POWER_UP_LIGHT_SPEED_ENGINES = (byte)0xFC;
    static final byte MSG_POWER_UP_SUBLIGHT_ENGINES = (byte)0xFB;
    static final byte MSG_POWER_UP_MAIN_SYSTEMS =(byte) 0xFA;
    static final byte MSG_UPDATE_AUDIO2_VOLUME =(byte) 0xF9;
    static final byte MSG_START_POWER_SEQUENCE =(byte) 0xF8;
    static final byte MSG_PLAY_VADER_INTRO =(byte) 0xF7;
    static final byte MSG_ENGINES_START_SEQUENCE =(byte) 0xF6;
    static final byte MSG_STOP_AUDIO =(byte) 0xF5;



    static final String ISD_BLE_DEVICE_NAME = "ISD Obliterator";
    static final UUID ISD_CHARACTERISTIC_UUID_HM10 = UUID.fromString("0000FFE1-0000-1000-8000-00805f9b34fb");
    static final UUID SERVICE_UUID_HM10 = UUID.fromString("0000FFE0-0000-1000-8000-00805f9b34fb");

    ScanCallback btScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {

            @SuppressLint("MissingPermission") String name = result.getDevice().getName();
            if (null != name) {
                name = name.trim();
            }
            else {
                return;
            }

            handler.post(new Runnable() {
                @Override
                public void run() {
                    ISDBluetoothController.this.onScan(result.getDevice());
                }
            });


            if (name.equals(ISD_BLE_DEVICE_NAME)) {

                ISDBluetoothController.this.deviceFound(result.getDevice());
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
            Log.w("bt","onScanFailed:" + errorCode);
            handler.post(new Runnable() {
                @Override
                public void run() {
                    ISDBluetoothController.this.scanFailed(errorCode);
                }
            });
        }
    };




    BluetoothGattCallback btDeviceCallback = new BluetoothGattCallback() {

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicRead(gatt, characteristic, status);
            Log.i("bt-io", "onCharacteristicRead status:" + status);
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gatt, characteristic, status);
            Log.i("bt-io", "onCharacteristicWrite status:" + status);
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            super.onServicesDiscovered(gatt, status);
            List<BluetoothGattService> services = gatt.getServices();

            Log.i("bt", "we're looking for service : " + SERVICE_UUID_HM10.toString());
            Log.i("bt", "we're looking for characteristic : " + ISD_CHARACTERISTIC_UUID_HM10.toString());

            for (BluetoothGattService service:services) {
                if (service.getUuid().equals(SERVICE_UUID_HM10)) {
                    List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
                    for (BluetoothGattCharacteristic characteristic:characteristics) {
                        Log.i("bt","chr:  " + characteristic.getUuid().toString());
                        if (characteristic.getUuid().equals(ISD_CHARACTERISTIC_UUID_HM10)) {
                            ISDBluetoothController.this.btWriteCharacteristic = characteristic;
                            ISDBluetoothController.this.writeType = characteristic.getWriteType();

                            Log.i("bt","ISDBluetoothController.this.btWriteCharacteristic service:  " + service.getUuid().toString());
                            Log.i("bt","ISDBluetoothController.this.btWriteCharacteristic:  " + ISDBluetoothController.this.btWriteCharacteristic.getUuid().toString());
                            Log.i("bt","ISDBluetoothController.this.writeType:  " +ISDBluetoothController.this.writeType);

                            handler.post(new Runnable() {
                                @Override
                                public void run() {
                                    ISDBluetoothController.this.delegate.deviceReady(ISDBluetoothController.this.btDevice);
                                }
                            });
                            break;
                        }
                    }
                    break;
                }
            }
        }

        @SuppressLint("MissingPermission")
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);
            switch ( status ) {
                case BluetoothGatt.GATT_SUCCESS: {
                    if (newState == BluetoothProfile.STATE_CONNECTED) {
                        Log.i("bt", "Connection: " + gatt.getDevice().getName());

                        btConnection = gatt;

                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                ISDBluetoothController.this.deviceConnected();
                            }
                        });
                    }
                    else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                        Log.i("bt", "Disconnected: " + gatt.getDevice().getName());

                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                ISDBluetoothController.this.deviceDisconnected();
                            }
                        });

                        gatt.close();
                    }
                }
                break;

                default : {
                    Log.w("bt","Error during connection state, status: " + status + " newState: " + newState);
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            ISDBluetoothController.this.deviceDisconnected();
                        }
                    });
                    gatt.close();
                }
                break;
            }

        }
    };




    public void init(Context context,ISDControllerUIDelegate delegate) {
        handler = new Handler();
        this.delegate = delegate;
        this.context = context;

        btManager = (BluetoothManager) this.context.getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();

        if (null == btAdapter) {
            Toast.makeText(context, "Error, Bluetooth not supported", Toast.LENGTH_SHORT).show();
            return;
        }

        ArrayList<String> perms = new ArrayList<>();
        if (ActivityCompat.checkSelfPermission(this.context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            perms.add(Manifest.permission.BLUETOOTH_CONNECT);
        }
        if (ActivityCompat.checkSelfPermission(this.context, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            perms.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }
        if (ActivityCompat.checkSelfPermission(this.context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            perms.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }

        if (perms.size()>0) {
            String[] permArr = perms.toArray(new String[perms.size()]);

            Activity activity = (Activity)context ;
            ActivityCompat.requestPermissions(activity, permArr, BT_PERMS_REQUEST);
        }
    }

    public void onAppCompatRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        permsGranted = false;
        if (BT_PERMS_REQUEST == requestCode) {
            permsGranted = true;
            for (int grant:grantResults) {
                if (grant!= PackageManager.PERMISSION_GRANTED) {
                    permsGranted = false;
                    break;
                }
            }
        }

        handler.post(new Runnable() {
            @Override
            public void run() {
                ISDBluetoothController.this.postPermsInit(permsGranted);
            }
        });
    }


    @SuppressLint("MissingPermission")
    void postPermsInit(boolean permsGranted) {
        if (!btAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);

            Activity activity = (Activity)context;
            activity.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }
        else {
            delegate.adapterEnabled(btAdapter);
        }

        btScanner = btAdapter.getBluetoothLeScanner();
        if (null == btScanner) {
            Toast.makeText(context, "Error, Bluetooth scanning not supported", Toast.LENGTH_SHORT).show();
            return;
        }

        btScanner.stopScan(btScanCallback);

        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                startScanning();
            }
        }, 500);
    }

    @SuppressLint("MissingPermission")
    private void deviceFound(BluetoothDevice device) {
        ISDBluetoothController.this.stopScanning();
        btDevice = device;

        btDevice.connectGatt(context, false, btDeviceCallback);

        handler.post(new Runnable() {
            @Override
            public void run() {
                delegate.deviceFound(btDevice);
            }
        });
    }

    private void deviceDisconnected() {
        delegate.deviceDisconnected();
    }

    @SuppressLint("MissingPermission")
    private void deviceConnected() {
        btConnection.discoverServices();
        delegate.deviceConnected();
    }

    private void scanFailed(int errorCode) {
        delegate.scanFailed(btScanner,errorCode);
    }

    public void stopScanning() {
        AsyncTask.execute(new Runnable() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
                btScanner.stopScan(btScanCallback);
                Log.i("bt","scan stop");
                scanningInProgress = false;

                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        ISDBluetoothController.this.delegate.scanStopped();
                    }
                });
            }
        });

    }

    private void onScan(BluetoothDevice device) {
    }
    
    public void startScanning() {
        AsyncTask.execute(new Runnable() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
                btScanner.startScan(btScanCallback);
                scanningInProgress = true;
                Log.i("bt","scan started...");

                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        ISDBluetoothController.this.delegate.scanStarted();
                    }
                });
            }
        });

        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (scanningInProgress) {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            ISDBluetoothController.this.stopScanning();
                        }
                    });
                }
            }
        }, 1000*getScanTimeout());
    }


    byte[] frameMessage( int msg, int arg ) {
        byte bytes[] = {STX,(byte)msg,(byte)arg,ETX};
        return bytes.clone();
    }


    @SuppressLint("MissingPermission")
    void btWriteMessage(int msg, int arg) {
        byte[] data = frameMessage(msg, arg);
        String s = String.format("0x%02x 0x%02x 0x%02x 0x%02x ",data[0], data[1], data[2], data[3]);
        Log.i("bt-msg", s);


        boolean isReady = false;
        if (btAdapter.isEnabled() ) {
            if (null != btConnection && null != btDevice) {
                isReady = true;
            }
        }

        if (!isReady) {
            return;
        }

        btWriteCharacteristic.setWriteType(writeType);
        btWriteCharacteristic.setValue(data);
        btConnection.writeCharacteristic(btWriteCharacteristic);


    }

    void btWriteMessage(int message) {
        btWriteMessage(message,  0);
    }


    public int getScanTimeout() {
        return scanTimeout;
    }

    public void setScanTimeout(int scanTimeout) {
        this.scanTimeout = scanTimeout;
    }

    @SuppressLint("MissingPermission")
    public void disconnectFromDevice() {
        if (null != btConnection) {
            btConnection.disconnect();
            btConnection.close();

            handler.post(new Runnable() {
                @Override
                public void run() {
                    ISDBluetoothController.this.delegate.deviceDisconnected();
                }
            });
        }

    }

    @SuppressLint("MissingPermission")
    public void connectToDevice() {
        btDevice.connectGatt(context, false, btDeviceCallback);
    }


    public void playImperialMarch() {
        btWriteMessage( MSG_PLAY_IMPERIAL_MARCH );
    }

    public void playVadersIntro() {
        btWriteMessage( MSG_PLAY_VADER_INTRO );
    }

    public void startPowerSequence(boolean withAudio) {
        btWriteMessage( MSG_START_POWER_SEQUENCE, withAudio ? ARG_WITH_AUDIO : 0x00 );
    }

    public void startEnginesSequence(boolean withAudio) {
        btWriteMessage( MSG_ENGINES_START_SEQUENCE, withAudio ? ARG_WITH_AUDIO : 0x00 );
    }

    public void changeAudio1Volume(int volume, int minVolume, int maxVolume) {
        double d = (double)(maxVolume - minVolume);
        double v = (double)(volume-minVolume);
        int mappedVol = (int)((v/d) * 30.0);
        mappedVol = (mappedVol < 0) ? 0 : (mappedVol>30?30:mappedVol);

        btWriteMessage( MSG_UPDATE_AUDIO_VOLUME, mappedVol );
    }

    public void changeAudio2Volume(int volume) {
        btWriteMessage( MSG_UPDATE_AUDIO_VOLUME, volume );
    }

    public void setGarbageChute(boolean val) {
        btWriteMessage( MSG_GARBAGE_CHUTE_ON, val ? 1:0 );
    }

    public void setMainSystemsPower(boolean val,boolean withAudio) {
        btWriteMessage( MSG_POWER_UP_MAIN_SYSTEMS, (val ? 1:0) | (withAudio ? ARG_WITH_AUDIO : 0x00) );
    }

    public void setMainEngines(boolean val,boolean withAudio) {
        btWriteMessage( MSG_POWER_UP_SUBLIGHT_ENGINES, (val ? 1:0) | (withAudio ? ARG_WITH_AUDIO : 0x00) );
    }

    public void setLightspeedEngines(boolean val,boolean withAudio) {
        btWriteMessage( MSG_POWER_UP_LIGHT_SPEED_ENGINES, (val ? 1:0) | (withAudio ? ARG_WITH_AUDIO : 0x00) );
    }

    public void stopAudio() {
        btWriteMessage( MSG_STOP_AUDIO );
    }

    public void rescan() {
        disconnectFromDevice();

        stopScanning();

        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                startScanning();
            }
        }, RESCAN_DELAY);
    }


}
