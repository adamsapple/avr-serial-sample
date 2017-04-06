using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using NotAmp.Protocol;

namespace SerialTest001
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {

        internal SerialPort serialPort1 = new SerialPort();

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {


            //! 利用可能なシリアルポート名を取得し、コンボボックスにセットする.
            comboPort.Items.Clear();

            foreach (string PortName in SerialPort.GetPortNames())
            {
                comboPort.Items.Add(PortName);
            }

            if (comboPort.Items.Count > 0)
            {
                comboPort.SelectedIndex = 0;
            }

            //! ボーレート選択コンボボックスに選択項目をセットする.
            int[] baudrates = { 4800, 9600, 19200, 115200 };
            comboBaudrate.Items.Clear();

            foreach (var rate in baudrates)
            {
                var baud      = new BuadRateItem();
                baud.BAUDRATE = rate;
                baud.NAME     = $"{baud.BAUDRATE}bps";
                comboBaudrate.Items.Add(baud);
            }   

            comboBaudrate.SelectedIndex = 1;


            //! フロー制御選択コンボボックスに選択項目をセットする.
            var dict = new Dictionary<string, Handshake>()
                {
                    {"なし"                   , Handshake.None},
                    {"XON/XOFF制御"           , Handshake.XOnXOff},
                    {"RTS/CTS制御"            , Handshake.RequestToSend},
                    {"XON/XOFF + RTS/CTS制御" , Handshake.RequestToSendXOnXOff}
                };
            comboHandShake.Items.Clear();

            foreach (KeyValuePair<string, Handshake> pair in dict)
            {
                var ctrl       = new HandShakeItem();
                ctrl.NAME      = pair.Key;
                ctrl.HANDSHAKE = pair.Value;
                comboHandShake.Items.Add(ctrl);
            }

            comboHandShake.SelectedIndex = 0;

            //! 送受信用テキストボックスのクリア
            textRecv.Clear();
            textSend.Clear();

            //! シリアルデータ受信イベント通知設定
            serialPort1.DataReceived += serialPort1_DataReceived_NAP;
        }

        private void serialPort1_DataReceived_NAP(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            if (serialPort1.IsOpen == false)                    //!< シリアルポートをオープンしていない場合、処理を行わない.
            {
                return;
            }

            if (serialPort1.BytesToRead < 8)
            {
                return;
            }

            try
            {
                byte[] data = new byte[8];
                serialPort1.Read(data, 0, 8);

                var msg = Message.ToStruct(data);

                var dispatcher = System.Windows.Application.Current.Dispatcher;
                dispatcher.BeginInvoke((Action)delegate ()
                {
                    textRecv.AppendText($"op: {msg.op} ({msg.val_c}, {msg.val_i_a}, {msg.val_i_b}, {msg.val_i_c})\n");

                    textRecv.SelectionStart = textRecv.Text.Length;
                    textRecv.ScrollToEnd();
                });
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void serialPort1_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            
            if (serialPort1.IsOpen == false)                    //!< シリアルポートをオープンしていない場合、処理を行わない.
            {
                return;
            }

            try
            {
                string data = serialPort1.ReadExisting();       //!< 受信データを読み込む.
                var dispatcher = System.Windows.Application.Current.Dispatcher;

                dispatcher.BeginInvoke((Action)delegate ()
                {
                    textRecv.Text += data;

                });
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void btnConnect_Click(object sender, RoutedEventArgs e)
        {
            if (serialPort1.IsOpen == true)
            {
                serialPort1.Close();                             //!< シリアルポートをクローズする.
                btnConnect.Content = "接続";                     //!< ボタンの表示を[切断]から[接続]に変える.
                return;
            }

            if (comboPort.Items.Count <= 0)
            {
                return;
            }


            textRecv.Clear();

            serialPort1.PortName = comboPort.SelectedItem.ToString();   //!< オープンするシリアルポートをコンボボックスから取り出す.

            var baud = (BuadRateItem)comboBaudrate.SelectedItem;        //!< ボーレートをコンボボックスから取り出す.
            serialPort1.BaudRate = baud.BAUDRATE;

            
            serialPort1.DataBits = 8;                                   //!< データビットをセットする. (データビット = 8ビット)
            serialPort1.Parity   = Parity.None;                         //!< パリティビットをセットする. (パリティビット = なし)
            serialPort1.StopBits = StopBits.One;                        //!< ストップビットをセットする. (ストップビット = 1ビット)
            serialPort1.Encoding = Encoding.ASCII;                      //!< 文字コードをセットする

            //! フロー制御をコンボボックスから取り出す.
            var ctrl = (HandShakeItem)comboHandShake.SelectedItem;
            serialPort1.Handshake = ctrl.HANDSHAKE;

            try
            {
                serialPort1.Open();                           //!< シリアルポートをオープンする.
                btnConnect.Content = "切断";                  //!< ボタンの表示を[接続]から[切断]に変える.
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void btnSend_Click(object sender, RoutedEventArgs e)
        {
            
            if (serialPort1.IsOpen == false)                //!< シリアルポートをオープンしていない場合、処理を行わない.
            {
                return;
            }
            
            var data = textSend.Text+"\n";          //!< テキストボックスから、送信するテキストを取り出す.

            //! 送信するテキストがない場合、データ送信は行わない.
            if (string.IsNullOrEmpty(data) == true)
            {
                return;
            }

            try
            {
                serialPort1.Write(data);            //!< シリアルポートからテキストを送信する.
                textSend.Clear();                   //!< 送信データを入力するテキストボックスをクリアする.
                textSend.Focus();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void SendNapMessage( Message msg )
        {
            if (serialPort1.IsOpen == false)        //!< シリアルポートをオープンしていない場合、処理を行わない.
            {
                return;
            }

            try
            {
                serialPort1.Write(msg);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void btnNapWAY_Click(object sender, RoutedEventArgs e)
        {
            SendNapMessage(new NotAmp.Protocol.Message("way"));
        }

        private void btnNapVER_Click(object sender, RoutedEventArgs e)
        {
            SendNapMessage(new NotAmp.Protocol.Message("ver"));
        }

        private void btnNapVOL_Click(object sender, RoutedEventArgs e)
        {
            SendNapMessage(new NotAmp.Protocol.Message("vol"));
        }

        private void btnNapMIC_Click(object sender, RoutedEventArgs e)
        {
            SendNapMessage(new NotAmp.Protocol.Message("mic"));
        }

        private void btnNapMPW_Click(object sender, RoutedEventArgs e)
        {
            SendNapMessage(new NotAmp.Protocol.Message("mpw"));
        }

        private void sliderPKM_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            //sliderPKM.Value = Math.Round(sliderPKM.Value);
            SendNapMessage(new NotAmp.Protocol.Message("pkm", (Int16)sliderPKM.Value, 0, 0));
        }
    }
}
